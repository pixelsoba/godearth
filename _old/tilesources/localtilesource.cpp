#include "mapwidget/tilesources/localtilesource.h"

#include <graphicsitems/geometryitem.h>

LocalTileWorker::LocalTileWorker(const QString& folderCacheName,
                                 const QString& tilePathTemplate)
    : TileWorker{},
      folderCacheName_{folderCacheName},
      tilePathTemplate_{tilePathTemplate} {}

void LocalTileWorker::operator()(TilePromise* promise) {
    auto* const localPromise = qobject_cast<LocalTilePromise*>(promise);
    const auto key = localPromise->tileKey().value<XYZ>();
    if (key.valid()) {
        if (!localPromise->aborted()) {
            auto path
                = QFileInfo(folderCacheName_ + "/"
                            + tilePathTemplate_.arg(key.z()).arg(key.x()).arg(
                                key.y()))
                      .absoluteFilePath();
            QImage image(path);
            if (!image.isNull())
                localPromise->loadPixmapData(image);
        }
    }
    localPromise->setFinished(true);
    emit finished(localPromise);
}

LocalTileSourceXYZ::LocalTileSourceXYZ(const QString& name,
                                       const QString& folderCacheName,
                                       const QString& tilePathTemplate,
                                       int minZoomLevel, int maxZoomLevel,
                                       int tileSize, const QString& legalString,
                                       bool alwaysUpdated)
    : LocalTileSourceAbstractXYZ{name,
                                 folderCacheName,
                                 tilePathTemplate,
                                 minZoomLevel,
                                 maxZoomLevel,
                                 tileSize,
                                 legalString,
                                 alwaysUpdated},
      extentsItemGroup_{new QGraphicsItemGroup},
      extentGeometryItem_{new PolygonItem()} {

    worker_ = new LocalTileWorker{folderCacheName, tilePathTemplate};
    this->connectTileWorker();

    GeometryItem::Style okStyle;
    okStyle["pen"] = QPen{Qt::green};
    QBrush brush;
    brush.setColor(QColor{0, 200, 0});
    okStyle["brush"] = brush;
    GeometryItem::StyleMap itemStyleMap;
    itemStyleMap[""] = okStyle;

    const auto itemName = QString{"%1 extent"}.arg(name);
    extentsItemGroup_->setData(CustomQt::ItemNameKey, itemName);
    extentsItemGroup_->setData(CustomQt::ItemStyleMap, itemStyleMap);

    extentGeometryItem_->setParentItem(extentsItemGroup_);
    extentGeometryItem_->setData(CustomQt::UserCantEditGeometry, true);
    extentGeometryItem_->setEdited(false);
}

QRectF LocalTileSourceXYZ::extentRect(int zLevel) {

    QDir zDir(folderCacheName_);

    std::vector<XYZ> tileKeys;
    QVector<QPair<QString, QString>> keys
        = {{"z", "%1"}, {"x", "%2"}, {"y", "%3"}};
    std::sort(
        keys.begin(), keys.end(),
        [this](QPair<QString, QString>& a, QPair<QString, QString>& b) -> bool {
            return tilePathTemplate_.indexOf(a.second)
                   < tilePathTemplate_.indexOf(b.second);
        });
    QString pattern = "(?<%1>\\d+)\\/(?<%2>\\d+)\\/(?<%3>\\d+)\\..";
    for (auto& k : keys) {
        pattern = pattern.arg(k.first);
    }

    QRegularExpression exp(pattern);

    QDirIterator it(zDir.absolutePath(), QDir::NoDotAndDotDot | QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        auto relPath = zDir.relativeFilePath(fileInfo.absoluteFilePath());

        auto match = exp.match(relPath);
        if (!match.isValid()) {
            continue;
        }
        auto x = match.captured("x").toInt();
        auto y = match.captured("y").toInt();
        auto z = match.captured("z").toInt();

        if (z == zLevel) {
            XYZ xyz(x, y, z);
            if (xyz.valid()) {
                tileKeys.push_back(xyz);
            }
        }
    }

    QRectF sceneRect;

    for (const auto& tileKey : tileKeys) {
        sceneRect |= tileKey.sceneRect();
    }

    return sceneRect;
}

QGraphicsItemGroup* LocalTileSourceXYZ::getExtentItem() {

    updateExtentItem();
    return extentsItemGroup_;
}

void LocalTileSourceXYZ::updateExtentItem() {
    auto rect = extentRect(maxZoomLevel_);
    if (!rect.isNull()) {
        OGRPolygon poly;
        OGRLinearRing lr;
        lr.addPoint(rect.left(), rect.top());
        lr.addPoint(rect.right(), rect.top());
        lr.addPoint(rect.right(), rect.bottom());
        lr.addPoint(rect.left(), rect.bottom());
        lr.closeRings();
        poly.addRing(&lr);

        extentGeometryItem_->setGeometry(&poly, Map::spatialReference());
    }
}

LocalTileSourceAbstractXYZ::LocalTileSourceAbstractXYZ(
    const QString& name, const QString& folderCacheName,
    const QString& tilePathTemplate, int minZoomLevel, int maxZoomLevel,
    int tileSize, const QString& legalString, bool alwaysUpdated)
    : QObject{},
      BaseType{name,        minZoomLevel,    maxZoomLevel,
               tileSize,    folderCacheName, tilePathTemplate,
               legalString, alwaysUpdated},
      worker_{nullptr} {

    thread_.start();
}

LocalTileSourceAbstractXYZ::~LocalTileSourceAbstractXYZ() {
    if (worker_) {
        disconnect(this, &LocalTileSourceAbstractXYZ::operate, worker_,
                   &TileWorker::operator());
        disconnect(worker_, &TileWorker::finished, this,
                   &LocalTileSourceAbstractXYZ::handleResult);
        delete worker_;
    }

    thread_.terminate();
    thread_.wait();
}

void LocalTileSourceAbstractXYZ::connectTileWorker() {

    if (!worker_) {
        return;
    }

    worker_->moveToThread(&thread_);
    connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(this, &LocalTileSourceAbstractXYZ::operate, worker_,
            &TileWorker::operator());
    connect(worker_, &TileWorker::finished, this,
            &LocalTileSourceAbstractXYZ::handleResult);
}

TilePromise*
LocalTileSourceAbstractXYZ::createTile(const QVariant& tileKey,
                                       TileItemGroup* group) const {
    auto* map = group ? group->map() : nullptr;
    if (!map)
        return nullptr;

    const auto key = tileKey.value<XYZ>();
    if (!key.valid())
        return nullptr;

    if (!folderCacheName_.isEmpty()) {
        auto path = QFileInfo(folderCacheName_ + "/"
                              + tilePathTemplate_.arg(key.z()).arg(key.x()).arg(
                                  key.y()))
                        .absoluteFilePath();

        if (!QFile(path).exists()) {
            return nullptr;
        }
    }

    // ignore if the tile is outside the extents (prevents generating empty
    // tiles)
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

void LocalTileSourceAbstractXYZ::handleResult(TilePromise* promise) {
    if (!promise)
        return;

    emit promise->finished();
}
