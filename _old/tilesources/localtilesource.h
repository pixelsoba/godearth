#pragma once

#include <memory>
#include <mutex>

#include <QPixmap>
#include <QThread>

#include "mapwidget/global.h"
#include "mapwidget/graphicsitems/tileitem.h"
#include "mapwidget/tilesources/tilepromise.h"
#include "mapwidget/tilesources/tilesourceimpl.h"
#include "mapwidget/tilesources/xyz.h"

class PolygonItem;

class MAPWIDGET_LIBRARY LocalTilePromise : public TilePromise {
    Q_OBJECT

public:
    LocalTilePromise(const QVariant& tileKey, TileItemGroup* group,
                     QObject* parent = nullptr)
        : TilePromise{tileKey, group, parent},
          aborted_{false},
          finished_{false} {}

    ~LocalTilePromise() override {}

    void loadPixmapData(const QImage& image) {
        if (image.isNull()) {
            return;
        }
        std::lock_guard<std::mutex> lck{access_};
        pixmap_.convertFromImage(image);
    }

    bool isFinished() const override {
        std::lock_guard<std::mutex> lck{access_};
        return finished_;
    }

    void setFinished(bool finished) {
        std::lock_guard<std::mutex> lck{access_};
        finished_ = finished;
    }

    bool aborted() const {
        std::lock_guard<std::mutex> lck{access_};
        return aborted_;
    }

    void abort() override {
        std::lock_guard<std::mutex> lck{access_};
        aborted_ = true;
    }

    TileItem* item() const override {
        std::lock_guard<std::mutex> lck{access_};
        if (!finished_ || aborted_ || pixmap_.isNull())
            return nullptr;

        auto* const item = new TileItem{group_, tileKey()};
        item->setPixmap(pixmap_);

        return item;
    }

private:
    QPixmap pixmap_;

    bool aborted_;

    bool finished_;

    mutable std::mutex access_;
};

class MAPWIDGET_LIBRARY LocalTileWorker : public TileWorker {
    Q_OBJECT

public:
    LocalTileWorker(const QString& folderCacheName = "",
                    const QString& tilePathTemplate = "");

    void operator()(TilePromise* promise) override;

private:
    QString folderCacheName_;
    QString tilePathTemplate_;
};

class MAPWIDGET_LIBRARY LocalTileSourceAbstractXYZ
    : public QObject,
      public TileSourceImpl<XYZ> {
    Q_OBJECT

    typedef TileSourceImpl<XYZ> BaseType;

public:
    LocalTileSourceAbstractXYZ(const QString& name,
                               const QString& folderCacheName = "",
                               const QString& tilePathTemplate = "",
                               int minZoomLevel = 0, int maxZoomLevel = 18,
                               int tileSize = 256,
                               const QString& legalString = "",
                               bool alwaysUpdated = false);

    ~LocalTileSourceAbstractXYZ() override;

    TilePromise* createTile(const QVariant& tileKey,
                            TileItemGroup* group) const override;

protected:
    TileWorker* worker_;

    void connectTileWorker();

    void handleResult(TilePromise* promise);

private:
    QThread thread_;

signals:
    void operate(TilePromise* promise) const;
};

class MAPWIDGET_LIBRARY LocalTileSourceXYZ : public LocalTileSourceAbstractXYZ {
    Q_OBJECT

public:
    LocalTileSourceXYZ(const QString& name, const QString& folderCacheName = "",
                       const QString& tilePathTemplate = "",
                       int minZoomLevel = 0, int maxZoomLevel = 18,
                       int tileSize = 256, const QString& legalString = "",
                       bool alwaysUpdated = false);


    QRectF extentRect(int zLevel);

    QGraphicsItemGroup* getExtentItem() override;

    void updateExtentItem() override;

private:
    QGraphicsItemGroup* const extentsItemGroup_;

    PolygonItem* const extentGeometryItem_;
};
