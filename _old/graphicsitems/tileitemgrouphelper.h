#pragma once

#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <QObject>
#include <QRectF>
#include <QVariant>
#include <QtGlobal> // qreal

#include "mapwidget/global.h"
#include "mapwidget/tilesources/tilesource.h"

class QNetworkReply;
class TileItem;
class TileItemGroup;
class TilePromise;
class MapView;
class Map;

#include "mapwidget/tilesources/xyz.h"
#include <cstdint>
#include <functional> // std::hash

// Hash specialization to allow storage of QVariant encapsulating XYZ
// instances in STL containers.
//
// It must be completed if tile items keys use other classes than XYZ.
namespace std {
template <> struct hash<QVariant> {
    typedef QVariant argument_type;
    typedef uint64_t result_type;

    result_type operator()(const argument_type& var) const {
        const auto xyz = var.value<XYZ>();
        if (xyz.valid()) {
            return std::hash<XYZ>()(xyz);
        }
        return 0;
    }
};
}

// Link a tile item group to a tile source
//
// It is responsible for requesting pixmaps, turning them into tiles,
// removing obsolete tiles and managing a tile index.
class MAPWIDGET_LIBRARY TileItemGroupHelper : public QObject {
    Q_OBJECT

public:
    typedef std::unordered_map<QVariant, TileItem*> TilesIndex;
    typedef std::unordered_map<QVariant, TilePromise*> PromiseIndex;

    TileItemGroupHelper(std::shared_ptr<TileSource> source,
                        TileItemGroup* group);

    ~TileItemGroupHelper() override;

    std::shared_ptr<TileSource> source() const { return source_; }

    bool mustDrawTileItem(TileItem* tile, qreal resolution) const;

    void
    fillWithTiles(const std::map<MapView*, std::pair<QRectF, qreal>>& rects);

    void abortPromises(const std::unordered_set<TilePromise*>* skip = nullptr);

private:
    std::shared_ptr<TileSource> source_;

    TilesIndex tiles_;

    const int marginSize_;

    PromiseIndex promises_;

    std::list<QVariant> obsoleteTiles_;

    TileItemGroup* const tilesGroup_;

    void requestTilesFor(QRect rect, int level,
                         std::unordered_set<TilePromise*>& keepPromises,
                         std::unordered_set<QVariant>& keepTiles);

    void
    identifyObsoleteTiles(const std::unordered_set<QVariant>& keepTiles,
                          const std::list<std::pair<QRect, int>>& tileRects);

    void removeObsoleteTiles();

    void onFinishedPromise();
};
