#include "mapwidget/tilesources/elevationtilesource.h"

#include <gdal.h>
#include <gdal_priv.h>
#include <gdalwarper.h>
#include <ogrsf_frmts.h>

#include <QGraphicsItemGroup>
#include <QPainter>

#include "mapwidget/3rdparty/gdal/gdaldem_lib.cpp"
#include "mapwidget/elevation/elevationhelper.h"
#include "mapwidget/graphicsitems/geometryitem.h"
#include "mapwidget/utilities/gdal.h"

ElevationTileWorker::ElevationTileWorker(int tileSize, ElevationHelper* helper)
    : TileWorker{},
      tileSize_{tileSize},
      helper_{helper},
      reliefColorsFile_{
          QStandardPaths::writableLocation(QStandardPaths::TempLocation)
          + "/relief_colors"} {
    Q_ASSERT(helper_);

    initColorsFile();
}

void ElevationTileWorker::operator()(TilePromise* promise) {

    auto* const localPromise = qobject_cast<LocalTilePromise*>(promise);
    const auto key = localPromise->tileKey().value<XYZ>();
    if (key.valid()) {
        const auto rect = key.sceneRect();
        if (!localPromise->aborted()) {
            auto path
                = QFileInfo(folderCacheName_ + "/"
                            + tilePathTemplate_.arg(key.z()).arg(key.x()).arg(
                                key.y()))
                      .absoluteFilePath();
            if (folderCacheName_.isEmpty() || !QFileInfo(path).exists()) {
                std::unique_ptr<GDALDataset> src{
                    helper_->beginWarpElevationFromScene(
                        rect, {tileSize_, tileSize_}, true)};
                auto* const srcBand = src ? src->GetRasterBand(1) : nullptr;
                if (srcBand) {

                    if (!localPromise->aborted()) {
                        QImage imgColorRelief = getColorReliefImg(src.get());
                        if (!localPromise->aborted()) {
                            QImage imgHillShade = getHillShadeImg(src.get());

                            QPainter p(&imgColorRelief);
                            p.setOpacity(0.4);
                            p.drawImage(0, 0, imgHillShade);

                            if (!imgColorRelief.isNull())
                                localPromise->loadPixmapData(imgColorRelief);
                            if (!path.isEmpty()) {
                                QDir().mkpath(QFileInfo(path).absolutePath());
                                imgColorRelief.save(path);
                            }
                        }
                    }
                }
                src.reset();
                helper_->endWarpElevation();
            } else {
                localPromise->loadPixmapData(QImage(path));
            }
        }
    }
    localPromise->setFinished(true);
    emit finished(localPromise);
}

void ElevationTileWorker::setFolderCacheName(const QString& folderCacheName) {
    folderCacheName_ = folderCacheName;
}

void ElevationTileWorker::setTilePathTemplate(const QString& tilePathTemplate) {
    tilePathTemplate_ = tilePathTemplate;
}

void ElevationTileWorker::initColorsFile() {
    if (reliefColorsFile_.open()) {

        double noDataValue = helper_->noDataValue();
        std::map<double, QColor> mapColors;

        mapColors.insert({noDataValue, QColor(Qt::red)});
        mapColors.insert({-500, QColor(100, 0, 255, 255)});
        mapColors.insert({0, QColor(41, 208, 209, 255)});
        mapColors.insert({1, QColor(240, 250, 150, 255)});
        mapColors.insert({100, QColor(50, 180, 50, 255)});
        mapColors.insert({1000, QColor(200, 150, 0, 255)});
        mapColors.insert({2000, QColor(77, 32, 0, 255)});
        mapColors.insert({3000, QColor(100, 100, 100, 255)});
        mapColors.insert({3500, QColor(255, 255, 255, 255)});

        QTextStream ts(&reliefColorsFile_);
        for (const auto& p : mapColors) {
            ts << (int)p.first << " " << p.second.red() << " "
               << p.second.green() << " " << p.second.blue() << " "
               << p.second.alpha() << "\n";
        }
        reliefColorsFile_.close();
    }
}

QImage ElevationTileWorker::getColorReliefImg(GDALDataset* src) const {
    auto* const srcBand = src ? src->GetRasterBand(1) : nullptr;
    if (!srcBand) {
        return QImage();
    }
    auto reliefDataset = new GDALColorReliefDataset(
        (GDALDatasetH)(src), (GDALRasterBandH)srcBand,
        reliefColorsFile_.fileName().toStdString().c_str(),
        COLOR_SELECTION_INTERPOLATE, true);

    const int bytes = tileSize_ * tileSize_ * 4;
    const int size = tileSize_ * tileSize_;
    auto uPData = std::make_unique<uchar[]>(bytes);
    auto* const data = uPData.get();
    const auto error = reliefDataset->RasterIO(
        GF_Read, 0, 0, tileSize_, tileSize_, data, tileSize_, tileSize_,
        GDT_Byte, reliefDataset->GetRasterCount(), nullptr, 0, 0, 0, nullptr);

    if (error != CE_None) {
        return QImage();
    }
    QColor color;
    QImage image{tileSize_, tileSize_, QImage::Format_RGBA8888};

    for (int i = 0; i < tileSize_; i++) {
        for (int j = 0; j < tileSize_; j++) {
            color.setRgb(data[i * tileSize_ + j],
                         data[i * tileSize_ + j + size],
                         data[i * tileSize_ + j + 2 * size]);
            color.setAlpha(data[i * tileSize_ + j + 3 * size]);
            image.setPixel(j, i, color.rgba());
        }
    }
    return image;
}

namespace HillShadeOptions {
constexpr int z = 6;
constexpr int scale = 1.0;
constexpr int az = 315.0;
constexpr int alt = 45.0;
// constexpr ColorSelectionMode eColorSelectionMode =
// COLOR_SELECTION_INTERPOLATE;
constexpr bool bComputeAtEdges = true;
constexpr bool bZevenbergenThorne = false;
constexpr bool bCombined = false;
} // namespace HillShadeOptions

QImage ElevationTileWorker::getHillShadeImg(GDALDataset* src) const {
    auto* const srcBand = src ? src->GetRasterBand(1) : nullptr;
    if (!srcBand) {
        return QImage();
    }
    std::array<double, 6> srcTrans;
    src->GetGeoTransform(srcTrans.data());

    struct CPLStringDeleter {
        void operator()(void* str) { CPLFree(str); }
    };
    std::unique_ptr<void, CPLStringDeleter> data{GDALCreateHillshadeData(
        srcTrans.data(), HillShadeOptions::z, HillShadeOptions::scale,
        HillShadeOptions::alt, HillShadeOptions::az,
        HillShadeOptions::bZevenbergenThorne)};

    GDALGeneric3x3ProcessingAlg alg = nullptr;
    if (HillShadeOptions::bZevenbergenThorne) {
        if (!HillShadeOptions::bCombined)
            alg = GDALHillshadeZevenbergenThorneAlg;
        else
            alg = GDALHillshadeZevenbergenThorneCombinedAlg;
    } else {
        if (!HillShadeOptions::bCombined)
            alg = GDALHillshadeAlg;
        else
            alg = GDALHillshadeCombinedAlg;
    }
    Q_ASSERT(alg);
    std::unique_ptr<GDALGeneric3x3Dataset> dst{new GDALGeneric3x3Dataset{
        src, srcBand, GDT_Byte, true, 0, alg, data.get(),
        HillShadeOptions::bComputeAtEdges}};
    auto* const dstBand = dst ? dst->GetRasterBand(1) : nullptr;

    if (!dst->InitOK() || !dstBand) {
        return QImage();
    }

    QImage image{tileSize_, tileSize_, QImage::Format_Grayscale8};
    const auto error
        = dstBand->RasterIO(GF_Read, 0, 0, tileSize_, tileSize_, image.bits(),
                            tileSize_, tileSize_, GDT_Byte, 0, 0, nullptr);
    if (error != CE_None) {
        return QImage();
    }

    return image;
}

ElevationTileSource::ElevationTileSource(ElevationHelper* helper,
                                         const QString& name,
                                         const QString& folderCacheName,
                                         const QString& tilePathTemplate,
                                         int minZoomLevel, int maxZoomLevel,
                                         int tileSize,
                                         const QString& legalString)
    : LocalTileSourceAbstractXYZ{name,
                                 folderCacheName,
                                 tilePathTemplate,
                                 minZoomLevel,
                                 maxZoomLevel,
                                 tileSize,
                                 legalString},
      helper_{helper} {

    worker_ = new ElevationTileWorker{tileSize, helper};
    static_cast<ElevationTileWorker*>(worker_)->setFolderCacheName(
        folderCacheName_);
    static_cast<ElevationTileWorker*>(worker_)->setTilePathTemplate(
        tilePathTemplate_);

    this->connectTileWorker();
}

std::shared_ptr<ElevationSource> ElevationTileSource::elevationSource() const {
    return helper_ ? helper_->source() : nullptr;
}

void ElevationTileSource::setFolderCacheName(const QString& folder) {
    LocalTileSourceAbstractXYZ::setFolderCacheName(folder);
    static_cast<ElevationTileWorker*>(worker_)->setFolderCacheName(folder);
}

void ElevationTileSource::setTilePathTemplate(const QString& tilePathTemplate) {
    LocalTileSourceAbstractXYZ::setTilePathTemplate(tilePathTemplate);
    static_cast<ElevationTileWorker*>(worker_)->setTilePathTemplate(
        tilePathTemplate);
}

QGraphicsItemGroup* ElevationTileSource::getExtentItem() {
    const auto source = helper_->source();
    auto index = source ? source->index() : nullptr;
    auto* const layer = index ? index->GetLayer(0) : nullptr;
    if (!layer)
        return nullptr;

    GeometryItem::Style okStyle;
    okStyle["pen"] = QPen{Qt::green};
    QBrush brush;
    brush.setColor(QColor{0, 200, 0});
    okStyle["brush"] = brush;
    GeometryItem::StyleMap itemStyleMap;
    itemStyleMap[""] = okStyle;

    auto* const extentItem = new QGraphicsItemGroup;
    const auto itemName = QString{"%1 extent"}.arg(name());
    extentItem->setData(CustomQt::ItemNameKey, itemName);
    extentItem->setData(CustomQt::ItemStyleMap, itemStyleMap);

    OGRSpatialReference srcSrs;
    if (layer->GetSpatialRef()) {
        srcSrs = *(layer->GetSpatialRef()); // mustn't be deleted!
    }
    srcSrs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    FeatureDeleter d;
    layer->ResetReading();
    while (std::unique_ptr<OGRFeature, FeatureDeleter&> f{
        layer->GetNextFeature(), d}) {
        auto* const geom = f->GetGeometryRef();
        if (geom) {
            auto* const item
                = GeometryItem::fromGeometry(geom, &srcSrs, extentItem);

            /* Ensure extent geometries cannot be selected/edited/removed */
            if (item) {
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                item->setData(CustomQt::UserCantEditGeometry, true);
                item->setData(CustomQt::UserCantRemove, true);
            }
        }
    }
    return extentItem;
}

TilePromise* ElevationTileSource::createTile(const QVariant& tileKey,
                                             TileItemGroup* group) const {
    auto* map = group ? group->map() : nullptr;
    if (!map)
        return nullptr;

    const auto key = tileKey.value<XYZ>();
    if (!key.valid())
        return nullptr;

    if (group->extentItem()) {
        auto sceneRect = group->extentItem()->childrenBoundingRect();
        if (!sceneRect.intersects(key.sceneRect())) {
            return nullptr;
        }
    }

    auto* promise = new LocalTilePromise{tileKey, group};
    emit operate(promise);

    return promise;
}

ElevationHelper* ElevationTileSource::helper() const { return helper_; }

void ElevationTileSource::clearCache() {
    QFileInfo info(folderCacheName_);
    QDir dir(info.absolutePath());
    dir.removeRecursively();
}
