#include "mapwidget/graphicsitems/rasteritem.h"

#include <array>
#include <cmath>
#include <functional>
#include <memory>

#include <cpl_progress.h>
#include <gdal.h>
#include <gdal_priv.h>

#include <QImage>
#include <QPainter>
#include <QRgb>
#include <QStyleOptionGraphicsItem>

#include "mapwidget/map.h"
#include "mapwidget/utilities/gdal.h"
#include "mapwidget/views/mapview.h"

// TODO read float bands
// TODO implement color maps
// TODO expose sampling algorithm
#define NODATA_8BITS 255
namespace {
void initializeColor(QColor& color, const GDALColorEntry& e,
                     GDALPaletteInterp interp) {
    switch (interp) {
    case GPI_Gray: {
        break;
    }
    case GPI_RGB: {
        color.setRgb(e.c1, e.c2, e.c3, e.c4);
        break;
    }
    case GPI_CMYK: {
        color.setCmyk(e.c1, e.c2, e.c3, e.c4);
        break;
    }
    case GPI_HLS: {
        color.setHsl(e.c1, e.c3, e.c2);
        break;
    }
    }
}

void scale2uchar(GDALRasterBand* const band, double* ddata, int rows, int cols,
                 uchar* cdata) {
    int hasNodataValue;
    auto ndataValue = band->GetNoDataValue(&hasNodataValue);
    double min = std::numeric_limits<double>::max();
    double max = -std::numeric_limits<double>::max();

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            auto const& v = ddata[i * cols + j];
            if (hasNodataValue && v == ndataValue) {
                continue;
            }

            if (v < min) {
                min = v;
            }

            if (v > max) {
                max = v;
            }
        }
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            auto v = ddata[i * cols + j];
            if (hasNodataValue && v == ndataValue) {
                cdata[i * cols + j] = NODATA_8BITS;
            } else {
                cdata[i * cols + j] = ((v - min) / (max - min)) * 254;
            }
        }
    }
}

void setImageData(uchar* data, int rows, int cols, int count, QImage* image) {
    Q_ASSERT(image);
    const int size = rows * cols;
    QColor color;
    if (count == 1) {
        if (image->format() == QImage::Format_RGBA8888) {
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    auto const& v = data[i * cols + j];
                    color.setRgb(v, v, v);
                    if (v == NODATA_8BITS) {
                        color.setAlpha(0);
                    }
                    image->setPixel(j, i, color.rgba());
                }
            }
        } else {
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    image->setPixel(j, i, data[i * cols + j]);
                }
            }
        }

    } else if (count == 3) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                color.setRgb(data[i * cols + j], data[i * cols + j + size],
                             data[i * cols + j + 2 * size]);
                image->setPixel(j, i, color.rgba());
            }
        }
    } else if (count == 4) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                color.setRgb(data[i * cols + j], data[i * cols + j + size],
                             data[i * cols + j + 2 * size]);
                color.setAlpha(data[i * cols + j + 3 * size]);
                image->setPixel(j, i, color.rgba());
            }
        }
    }
}

int trackProgress(double percent, const char*, void* arg) {
    RasterDataWorker* const worker = reinterpret_cast<RasterDataWorker*>(arg);
    if (worker->aborted()) {
#ifdef DEBUG_RASTER_ITEM
        qDebug() << "Abort promise";
#endif
        return 0;
    }
    worker->notifyProgress(percent);

    return 1;
}

const int userInteractionDelay = 500; // ms
} // namespace

RasterDataPromise::RasterDataPromise(const PixmapKey& key)
    : pixmap_{nullptr}, key_{key}, state_{Waiting}, detail_{Empty} {}

RasterDataPromise::~RasterDataPromise() { delete pixmap_; }

RasterDataPromise::State RasterDataPromise::state() const { return state_; }

RasterDataPromise::StateDetail RasterDataPromise::stateDetail() const {
    return detail_;
}

void RasterDataPromise::setState(RasterDataPromise::State state,
                                 RasterDataPromise::StateDetail detail) {
    if (state_ == state)
        return;

    state_ = state;
    detail_ = detail;
}

RasterItem::RasterItem(std::shared_ptr<GDALDataset> dataset)
    : QGraphicsObject{},
      map_{nullptr},
      listener_{nullptr},
      dataset_{dataset},
      timer_{new QTimer{this}},
      worker_{nullptr} {
    bool leaveItemEmpty = false;
    if (!dataset_) {
        leaveItemEmpty = true;
    } else {
        if (dataset_->GetGeoTransform(trans_.data()) != CE_None)
            leaveItemEmpty = true;

        const auto bandCount = dataset_->GetRasterCount();
        if (!bandCount)
            leaveItemEmpty = true;

        const char* wkt = dataset_->GetProjectionRef();
        OGRSpatialReference srs;
        auto* const mapSrs = Map::spatialReference();
        if (!wkt || srs.importFromWkt(&wkt) != OGRERR_NONE
            || !srs.IsSame(mapSrs))
            leaveItemEmpty = true;
    }
    if (leaveItemEmpty) {
        setFlag(QGraphicsItem::ItemHasNoContents, true);
    } else {
        rasterSize_
            = QSize{dataset_->GetRasterXSize(), dataset_->GetRasterYSize()};

        const double topLeftX = trans_[0];
        const double pixelWidth = trans_[1];
        const double topLeftY = trans_[3];
        const double pixelHeight = trans_[5]; // <0 for north-up images
        Q_ASSERT(trans_[2] == 0 && trans_[4] == 0);
        Q_ASSERT(pixelWidth != 0 && pixelHeight != 0);
        boundingRect_ = QRectF{0, 0, pixelWidth * rasterSize_.width(),
                               std::fabs(pixelHeight) * rasterSize_.height()};

        setPos(topLeftX + pixelWidth / 2.0, topLeftY + pixelHeight / 2.0);
        // scene coordinates since item has no parent

        setTransform(QTransform::fromScale(1, -1), false);

        using namespace std::placeholders;
        listener_ = std::bind(&RasterItem::scheduleRasterRead, this, _1);

        worker_ = new RasterDataWorker{dataset_};
        worker_->moveToThread(&thread_);
        connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
        connect(this, &RasterItem::operate, worker_,
                &RasterDataWorker::operator());
        connect(worker_, &RasterDataWorker::finished, this,
                &RasterItem::handleResult);
        connect(worker_, &RasterDataWorker::progress, this,
                &RasterItem::workerProgress);
        thread_.start();

        timer_->setInterval(userInteractionDelay);
        timer_->setSingleShot(true);
        connect(timer_, &QTimer::timeout, this,
                &RasterItem::launchScheduledPromises);
    }
}

RasterItem::~RasterItem() {
    if (map_ && listener_) {
        map_->unregisterViewListener(&listener_);
    }
    worker_->abort();
    thread_.quit();
    thread_.wait();

    for (auto& p : promises_) {
        delete p;
    }
}

QVariant RasterItem::itemChange(GraphicsItemChange change,
                                const QVariant& value) {
    switch (change) {
    case QGraphicsItem::ItemSceneChange: {
        // at this stage, scene is not set
        auto* const scene = qvariant_cast<QGraphicsScene*>(value);
        auto* const sceneMap
            = scene ? qobject_cast<Map*>(scene->parent()) : nullptr;
        if (sceneMap != map_) {
            if (map_)
                map_->unregisterViewListener(&listener_);
            map_ = sceneMap;
            if (map_ && isVisible())
                map_->registerViewListener(&listener_);
        }
        break;
    }
    case QGraphicsItem::ItemVisibleChange: {
        if (map_) {
            const bool visible = value.toBool();
            if (visible) {
                map_->registerViewListener(&listener_);
                map_->triggerViewChanged();
            } else {
                map_->unregisterViewListener(&listener_);
            }
        }
        break;
    }
    default:
        break;
    }
    return QGraphicsItem::itemChange(change, value);
}

void RasterItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*,
                       QWidget* widget) {
    if (!painter || !widget)
        return;

    const auto* const view = static_cast<MapView*>(widget->parent());
    const auto* const map = view->map();
    if (!map || !view->isVisible())
        return;

    const auto res = view->resolution();
    const auto viewSceneRect = view->currentSceneRect();
    auto* const scene = map->scene();
    const auto clipped = viewSceneRect.intersected(scene->sceneRect());
    const auto key = sceneRectKey(clipped, res);
    const auto s = std::find_if(
        std::begin(promises_), std::end(promises_),
        [&key](RasterDataPromise* p) {
            return (p && p->key() == key
                    && p->state() == RasterDataPromise::Finished
                    && p->stateDetail() == RasterDataPromise::Empty);
        });
    if (s != std::end(promises_)) {
        QPixmap* const p = (*s)->pixmap();
        if (p && !p->isNull()) {
            const bool hasAntialising
                = painter->renderHints() & QPainter::Antialiasing;
            if (hasAntialising) {
                painter->setRenderHint(QPainter::Antialiasing, false);
            }
            const auto itemRect = std::get<2>(key);
            painter->drawPixmap(itemRect, *p, p->rect());

            if (hasAntialising) {
                painter->setRenderHint(QPainter::Antialiasing, true);
            }
        }
    }
}

void RasterItem::scheduleRasterRead(
    const std::map<MapView*, std::pair<QRectF, qreal>>& rects) {
    if (flags() & QGraphicsItem::ItemHasNoContents)
        return;

    // identify new rectangles and rectangles that must be kept
    PixmapPromiseIndex keep;
    QRectF viewSceneRect;
    double res;
    for (auto& p : rects) {
        std::tie(viewSceneRect, res) = p.second;
        const auto key = sceneRectKey(viewSceneRect, res);
        const auto& srcRect = std::get<0>(key);
        const auto& dstRect = std::get<1>(key);
        if (srcRect.width() <= 0 || srcRect.height() <= 0
            || dstRect.width() <= 0 || dstRect.height() <= 0) {
            continue;
        }
        const auto s = std::find_if(
            std::begin(promises_), std::end(promises_),
            [&key](RasterDataPromise* p) { return p && p->key() == key; });
        if (s != std::end(promises_)) {
            keep.insert(std::end(keep), *s);
        } else {
            auto* const view = p.first;
            const auto t = schedule_.find(view);
            if (t == std::end(schedule_)) {
                schedule_.insert(
                    std::make_pair(view, new RasterDataPromise{key}));
            } else {
                auto* const previous = t->second;
#ifdef DEBUG_RASTER_ITEM
                const auto previousKey = previous->key();
                qDebug() << "Skip scheduled reading" << std::get<0>(previousKey)
                         << "into" << std::get<1>(previousKey) << "for"
                         << std::get<2>(previousKey);
#endif
                if (previous->state() != RasterDataPromise::Running) {
                    previous->deleteLater();
                }
                t->second = new RasterDataPromise{key};
            }
        }
    }
    deleteObsoletePromises(keep);

    timer_->start();
}

void RasterItem::deleteObsoletePromises(const PixmapPromiseIndex& keep) {
    for (auto* p : promises_) {
        Q_ASSERT(p);

        if (p->state() == RasterDataPromise::Finished
            || p->state() == RasterDataPromise::Running) {
            const auto key = p->key();
            const auto s = std::find_if(std::begin(keep), std::end(keep),
                                        [&key](RasterDataPromise* p) {
                                            return p && (p->key() == key);
                                        });
            if (s == std::end(keep)) {
                if (p->state() == RasterDataPromise::Finished) {
                    p->deleteLater();
                } else if (p->state() == RasterDataPromise::Running) {
                    Q_ASSERT(worker_);
                    worker_->abort();
                }
            }
        }
    }
    promises_.clear();
    promises_.insert(std::begin(keep), std::end(keep));
}

void RasterItem::launchScheduledPromises() {
    for (auto& p : schedule_) {
        auto* const promise = p.second;
        Q_ASSERT(promise);
        emit operate(promise);

        promises_.insert(promise);

#ifdef DEBUG_RASTER_ITEM
        const auto key = promise->key();
        qDebug() << "Schedule reading" << std::get<0>(key) << "into"
                 << std::get<1>(key) << "for" << std::get<2>(key);
#endif
    }
    schedule_.clear();
}

void RasterItem::handleResult(RasterDataPromise* promise) {
    Q_ASSERT(promise);
    Q_ASSERT(promise->state() == RasterDataPromise::Finished);
#ifdef DEBUG_RASTER_ITEM
    switch (promise->stateDetail()) {
    case RasterDataPromise::Empty:
        qDebug() << "Raster reading successful";
        break;
    case RasterDataPromise::Cancelled:
        qDebug() << "Raster reading cancelled";
        break;
    case RasterDataPromise::Failure:
        qDebug() << "Raster reading failed";
        break;
    default:
        qDebug() << "Unexpected promise state details!";
    }
#endif
    const auto key = promise->key();
    auto s = std::find_if(
        std::begin(promises_), std::end(promises_),
        [&key](RasterDataPromise* p) { return p && p->key() == key; });
    if (s == std::end(promises_)) {
        delete promise;
    } else {
        if (promise->stateDetail() == RasterDataPromise::Empty) {
            prepareGeometryChange();
        } else {
            promise->deleteLater();

            promises_.erase(s);
        }
    }
}

QPoint RasterItem::scene2image(const QPointF& p) const {
    const double topLeftX = trans_[0];
    const double pixelWidth = trans_[1];
    const double topLeftY = trans_[3];
    const double pixelHeight = trans_[5];
    Q_ASSERT(trans_[2] == 0 && trans_[4] == 0);
    Q_ASSERT(pixelWidth != 0 && pixelHeight != 0);

    // TODO Use geo2pixel() from utilities/gdal.h?

    return QPoint{
        static_cast<int>(std::trunc((p.x() - topLeftX) / pixelWidth)),
        static_cast<int>(std::trunc((p.y() - topLeftY) / pixelHeight))};
}

// returns the given scene rectangle intersected with the raster
// bounding box in both image coordinates and item coordinates
PixmapKey RasterItem::sceneRectKey(QRectF viewSceneRect, double res) const {
    const auto rasterSceneRect = mapRectToScene(boundingRect_);
    // translate bounding rect
    viewSceneRect = viewSceneRect.intersected(rasterSceneRect);

    QRect imageRect{scene2image(viewSceneRect.bottomLeft()),
                    scene2image(viewSceneRect.topRight())};
    // north-up images have a negative pixel height, thus scene rect
    // bottom left corner becomes image rect top left corner
    const QRect rasterRect{QPoint{0, 0}, rasterSize_};
    imageRect = imageRect.intersected(rasterRect);

    const auto viewSize = viewSceneRect.size() / res;
    const QSize dstSize{static_cast<int>(std::trunc(viewSize.width())),
                        static_cast<int>(std::trunc(viewSize.height()))};

    const auto itemRect = mapRectFromScene(viewSceneRect);

    return std::make_tuple(imageRect, dstSize, itemRect);
}

RasterDataWorker::RasterDataWorker(std::shared_ptr<GDALDataset> dataset)
    : QObject{},
      dataset_{dataset},
      bandCount_{dataset_ ? dataset_->GetRasterCount() : 0},
      abort_{false} {
    Q_ASSERT(dataset_);

    connect(this, &RasterDataWorker::wasAborted, this,
            &RasterDataWorker::resetAbortState);
}

void RasterDataWorker::operator()(RasterDataPromise* promise) {
    Q_ASSERT(promise);
    Q_ASSERT(promise->state() == RasterDataPromise::Waiting);

    promise->setState(RasterDataPromise::Running);

    const auto key = promise->key();
    const auto srcRect = std::get<0>(key);
    const auto dstRect = std::get<1>(key);

#ifdef DEBUG_RASTER_ITEM
    qDebug() << "Reading" << srcRect << "into" << dstRect;
#endif
    if (abort_) {
        promise->setState(RasterDataPromise::Finished,
                          RasterDataPromise::Cancelled);
        emit finished(promise);
        return;
    }
    QImage::Format format{QImage::Format_Invalid};
    std::unique_ptr<QImage> image;
    CPLErr error = CE_None;

    using namespace std::placeholders;
    GDALRasterIOExtraArg p;
    INIT_RASTERIO_EXTRA_ARG(p);
    p.pfnProgress = &trackProgress;
    p.pProgressData = this;

    int bandCount = bandCount_;
    if (bandCount == 2) {
        bandCount = 1;
    }

    switch (bandCount) {
    case 1: {
        auto* const band = dataset_->GetRasterBand(1);
        Q_ASSERT(band);
        auto* table = band->GetColorTable(); // owned by band, don't delete

        QImage::Format format
            = table ? QImage::Format_Indexed8 : QImage::Format_RGBA8888;

        image.reset(new QImage{dstRect.width(), dstRect.height(), format});

        const double noDataValue = band->GetNoDataValue(); // TODO handle status
        if (format == QImage::Format_Indexed8) {
            Q_ASSERT(table);
            const auto interp = table->GetPaletteInterpretation();
            for (int i = 0; i < table->GetColorEntryCount(); ++i) {
                auto* entry = table->GetColorEntry(i);
                if (entry) {
                    QColor color;
                    if (i == noDataValue) {
                        color.setAlpha(0);
                    } else {
                        initializeColor(color, *entry, interp);
                    }
                    image->setColor(i, color.rgba());
                }
            }
        }
        break;
    }
    case 3:
    case 4: {
        format = QImage::Format_RGBA8888;
        image.reset(new QImage{dstRect.width(), dstRect.height(), format});
        break;
    }
    default:
#ifdef DEBUG_RASTER_ITEM
        qDebug() << "Unexpected band number";
#endif
        promise->setState(RasterDataPromise::Finished,
                          RasterDataPromise::Failure);
        return;
    }
    if (error != CE_None || !image) {
        promise->setState(RasterDataPromise::Finished,
                          RasterDataPromise::Failure);
        emit finished(promise);
        return;
    }
    const int bytes = dstRect.width() * dstRect.height() * bandCount;
    auto data = std::make_unique<uchar[]>(bytes);

    bool read = false;
    if (bandCount == 1) {
        auto* const band = dataset_->GetRasterBand(1);
        if (band->GetRasterDataType() != GDT_Byte) {
            auto dataDouble = std::make_unique<double[]>(bytes);
            error = dataset_->RasterIO(
                GF_Read, srcRect.left(), srcRect.top(), srcRect.width(),
                srcRect.height(), dataDouble.get(), dstRect.width(),
                dstRect.height(), GDT_Float64, bandCount, nullptr, 0, 0, 0, &p);

            scale2uchar(band, dataDouble.get(), dstRect.height(),
                        dstRect.width(), data.get());
            read = true;
        }
    }
    if (!read) {
        error = dataset_->RasterIO(
            GF_Read, srcRect.left(), srcRect.top(), srcRect.width(),
            srcRect.height(), data.get(), dstRect.width(), dstRect.height(),
            GDT_Byte, bandCount, nullptr, 0, 0, 0, &p);
    }

    if (abort_) {
        promise->setState(RasterDataPromise::Finished,
                          RasterDataPromise::Cancelled);
        emit finished(promise);
        return;
    }

    setImageData(data.get(), dstRect.height(), dstRect.width(), bandCount,
                 image.get());
    if (bandCount_ == 2 && image->format() == QImage::Format_RGBA8888) {
        // Add alpha
        auto* const band = dataset_->GetRasterBand(2);
        error = band->RasterIO(GF_Read, srcRect.left(), srcRect.top(),
                               srcRect.width(), srcRect.height(), data.get(),
                               dstRect.width(), dstRect.height(), GDT_Byte, 0,
                               0, nullptr);
        int rows = dstRect.height();
        int cols = dstRect.width();
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                auto const& v = data.get()[i * cols + j];
                auto color = image->pixelColor(j, i);
                color.setAlpha(v);
                image->setPixel(j, i, color.rgba());
            }
        }
    }

    if (error != CE_None || !image) {
        promise->setState(RasterDataPromise::Finished,
                          RasterDataPromise::Failure);
        emit finished(promise);
        return;
    }
    promise->pixmap_ = new QPixmap;
    promise->pixmap_->convertFromImage(*image);

    promise->setState(RasterDataPromise::Finished);
    emit finished(promise);
}

void RasterDataWorker::notifyProgress(double percent) {
    emit progress(percent);
}

void RasterDataWorker::abort() {
    abort_ = true;

#ifdef DEBUG_RASTER_ITEM
    qDebug() << "Aborting enqueued promises";
#endif

    emit wasAborted();
    // enqueue a `was aborted' event into the worker's event loop to
    // abort all promises emitted to the worker's then restore the
    // aborted state
}

bool RasterDataWorker::aborted() const { return abort_; }

void RasterDataWorker::resetAbortState() {
    abort_ = false;
#ifdef DEBUG_RASTER_ITEM
    qDebug() << "Stop aborting enqueued promises";
#endif
}
