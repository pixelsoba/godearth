#pragma once

#include <map>

#include <QObject>
#include <QString>
#include <QVariant>

#include "mapwidget/global.h"

class TileItem;
class TileItemGroup;
class TilePromise;
class QGraphicsItemGroup;

class MAPWIDGET_LIBRARY TileWorker : public QObject {
    Q_OBJECT

public:
    TileWorker() : QObject{} {}
    ~TileWorker() {}

    virtual void operator()(TilePromise* promise) = 0;

signals:
    void finished(TilePromise* promise);
};

// Tile source don't inherit QObject to be able to have templated
// implementation. That's the reason why their memory management is
// done using shared pointers. Note that, at the present time,
// ElevationTileSource inherit QObject and is encapsulated in a share
// pointer, ugly.
class MAPWIDGET_LIBRARY TileSource {

public:
    TileSource(const QString& name, int minZoomLevel = 0, int maxZoomLevel = 28,
               int tileSize = 256, const QString& folderCacheName = "",
               const QString& tilePathTemplate = "",
               const QString& legalString = "", bool alwaysUpdated = false);

    virtual ~TileSource() {}

    QString name() const { return name_; }

    QString folderCacheName() const { return folderCacheName_; }

    virtual void setFolderCacheName(const QString& folder) {
        folderCacheName_ = folder;
    }

    QString tilePathTemplate() const { return tilePathTemplate_; }

    virtual void setTilePathTemplate(const QString& tilePathTemplate) {
        tilePathTemplate_ = tilePathTemplate;
    }

    QString legalString() const { return legalString_; }

    int minZoomLevel() const { return minZoomLevel_; }

    int maxZoomLevel() const { return maxZoomLevel_; }

    void setMinMaxZoomLevel(int zoomMin, int zoomMax);

    int tileSize() const { return tileSize_; }

    bool alwaysUpdated() const { return alwaysUpdated_; }

    int level(qreal resolution) const; // zoom level

    qreal resolution(int level) const; // m/px

    // TODO level and resolutions should be abstract, no?

    virtual QVariant tileKey(const QPointF& scenePos, int level,
                             bool* valid = nullptr) const = 0;

    virtual QVariant tileKey(const QPoint& tileCoord, // in given level plane
                             int level, bool* valid = nullptr) const = 0;

    virtual QPoint tileCoord(const QPointF& scenePos, int level,
                             bool* valid = nullptr) const = 0;

    virtual QPoint tileCoord(const QVariant& tileKey,
                             bool* valid = nullptr) const = 0;

    virtual QVariantList childTileKeys(const QVariant& tileKey) const = 0;

    virtual QVariant parentTileKey(const QVariant& tileKey) const = 0;

    virtual bool tileIntersect(const QVariant& tileKey, const QRect& tileRect,
                               int level, int levelDepth = -1) const = 0;

    // Warning: Releasing promise memory is under the responsability
    // of the caller
    virtual TilePromise* createTile(const QVariant& tileKey,
                                    TileItemGroup* group) const = 0;

    virtual QRect extent(int level) const = 0; // projection of valid tile
                                               // keys in the given level plane

    // Returned item owned by the caller
    virtual QGraphicsItemGroup* getExtentItem() { return nullptr; }

    // Tiles cache
    //
    // Default implementation return an empty cache path, thus tiles
    // won't be cached. By the way, remember that source must be
    // registered against tile cache for tiles to have a chance to be
    // cached.
    virtual QString cachePath(const QUrl& /* url */,
                              const QString& /* cacheRootPath */) const {
        return QString{};
    }

    virtual int cacheDelay() const // secs
    {
        return -1;
    }

    virtual void prepareCacheLayout(const QString& /* cacheRootPath */) const {}

    virtual void updateExtentItem() {}

protected:
    const QString name_;

    QString folderCacheName_;

    QString tilePathTemplate_;

    const QString legalString_;

    std::map<int, qreal> resolutions_;

    std::map<qreal, int> levels_;

    const int tileSize_;

    int minZoomLevel_;

    int maxZoomLevel_;

    bool alwaysUpdated_;

    void populateResolutions(int minZoomLevel, int maxZoomLevel);

    void populateLevels();
};
