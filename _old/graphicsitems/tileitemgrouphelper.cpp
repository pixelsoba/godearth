#include "mapwidget/graphicsitems/tileitemgrouphelper.h"

#include "mapwidget/graphicsitems/tileitem.h"
#include "mapwidget/graphicsitems/tileitemgroup.h"

#include "mapwidget/map.h"

#include "mapwidget/tilesources/tilepromise.h"
#include "mapwidget/tilesources/tilesource.h"

#include "mapwidget/views/mapview.h"

TileItemGroupHelper::TileItemGroupHelper(std::shared_ptr<TileSource> source,
                                         TileItemGroup* group)
    : QObject{nullptr}, source_{source}, marginSize_{1}, tilesGroup_{group} {
    Q_ASSERT(source);
    Q_ASSERT(tilesGroup_);
}

TileItemGroupHelper::~TileItemGroupHelper() {
    abortPromises();

    for (auto p : tiles_) {
        auto* const t = p.second;

        delete t; // item automatically removed from
                  // scene on deletion
    }
}

bool TileItemGroupHelper::mustDrawTileItem(TileItem* tile, qreal res) const {
    if (!tile || tile->parentItem() != tilesGroup_)
        return false;

    const auto tileKey = tile->tileKey();
    const auto tileLevel = tile->level();
    const auto level = source_->level(res);
    if (tileLevel <= level && level < tileLevel + 1) {
        return true;
    } else if (tileLevel == level - 1) {
        // Display tile if one of its child is missing
        for (const auto& child : source_->childTileKeys(tileKey)) {
            const auto s = tiles_.find(child);
            if (s == tiles_.end())
                return true; // missing subtile
        }
    } else if (level == tileLevel - 1) {
        // Display tile if its parent is missing
        const auto parent = source_->parentTileKey(tileKey);
        const auto s = tiles_.find(parent);
        if (s == tiles_.end())
            return true;
    }
    // TODO move to a zoom policy (StrictZoom or ZoomOnTiles)
    return false;
}

void TileItemGroupHelper::fillWithTiles(

    const std::map<MapView*, std::pair<QRectF, qreal>>& rects) {
    auto* const map = tilesGroup_->map();
    if (!map)
        return;

    std::unordered_set<TilePromise*> keepPromises;
    std::unordered_set<QVariant> keepTiles;
    std::list<std::pair<QRect, int>> tileRects;

    bool valid = false;
    auto it = rects.begin();
    while (it != rects.end()) {
        const auto res = it->second.second;
        const int closestLevel = source_->level(res);
        auto viewSceneRect = it->second.first;
        int minLevel = std::max(0, source_->minZoomLevel() - 1);

        if (tilesGroup_->extentItem()) {
            MapView* mapview = it->first;
            auto sourceExtenRect
                = tilesGroup_->extentItem()->childrenBoundingRect();
            if (sourceExtenRect.isValid()) {
                viewSceneRect
                    &= sourceExtenRect; // intersect with the source extent to
                                        // avoid looping on invalid or unneeded
                                        // tile keys

                auto fitLevel = mapview->fitZoomLevel(sourceExtenRect);
                minLevel = std::min(fitLevel, minLevel);
            }
        }

        // If the requested level is inferior to the minimum zoom level, we
        // end up requesting too many tiles that are not needed and causing
        // the application to freeze.

        int requestedLevel
            = std::log2(Map::earthPerimeter_ / source_->tileSize() / res);

        if (requestedLevel < minLevel) {
            it++;
            for (auto& tile : tiles_) {
                keepTiles.insert(tile.first); // keep displayed tiles
            }
            for (auto& promise : promises_) {
                keepPromises.insert(promise.second); // keep ongoing prom
            }

            continue;
        }

        QPolygon p;
        QPoint tileCoord;
        tileCoord
            = source_->tileCoord(viewSceneRect.topLeft(), closestLevel, &valid);
        if (valid) {
            p.push_back(tileCoord);
        }
        tileCoord = source_->tileCoord(viewSceneRect.topRight(), closestLevel,
                                       &valid);
        if (valid) {
            p.push_back(tileCoord);
        }
        tileCoord = source_->tileCoord(viewSceneRect.bottomRight(),
                                       closestLevel, &valid);
        if (valid) {
            p.push_back(tileCoord);
        }
        tileCoord = source_->tileCoord(viewSceneRect.bottomLeft(), closestLevel,
                                       &valid);
        if (valid) {
            p.push_back(tileCoord);
        }
        auto rect = p.boundingRect();
        if (marginSize_) {
            const QMargins m{marginSize_, marginSize_, marginSize_,
                             marginSize_};
            rect += m;
        }
        rect &= source_->extent(closestLevel); // intersect with the source
                                               // extent to avoid looping on
                                               // invalid or unneeded tile keys

        requestTilesFor(rect, closestLevel, keepPromises, keepTiles);

        tileRects.push_back(std::make_pair(rect, closestLevel));

        ++it;
    }
    identifyObsoleteTiles(keepTiles, tileRects);
    abortPromises(&keepPromises);
}

void TileItemGroupHelper::requestTilesFor(
    QRect rect, int level, std::unordered_set<TilePromise*>& keepPromises,
    std::unordered_set<QVariant>& keepTiles) {

    auto* const map = tilesGroup_->map();
    if (!map)
        return;

    // Protection if the size of tiles is too big. Will freeze the app or even
    // crash it
    if (rect.width() * rect.height() > 1000) {
        return;
    }

    bool valid = false;
    for (int x = 0; x <= rect.width(); ++x) {
        for (int y = 0; y <= rect.height(); ++y) {
            const QPoint tileCoord{rect.x() + x, rect.y() + y};
            const auto tileKey = source_->tileKey(tileCoord, level, &valid);
            if (!valid)
                continue;

            const auto s = tiles_.find(tileKey);
            if (s != tiles_.end()) {
                keepTiles.insert(tileKey);
                if (!source_->alwaysUpdated()) {
                    continue;
                }
            }

            const auto t = promises_.find(tileKey);
            if (t != promises_.end()) {
                keepPromises.insert(t->second);
                continue;
            }

            auto* const promise = source_->createTile(tileKey, tilesGroup_);
            if (promise) {
                PromiseIndex::iterator it;
                bool inserted;
                std::tie(it, inserted)
                    = promises_.insert(std::make_pair(tileKey, promise));
                if (inserted)
                    keepPromises.insert(promise);

                connect(promise, &TilePromise::finished, this,
                        &TileItemGroupHelper::onFinishedPromise);
            }
        }
    }
}

void TileItemGroupHelper::abortPromises(
    const std::unordered_set<TilePromise*>* skip) {
    std::unordered_set<TilePromise*> toAbort;
    PromiseIndex::iterator it = promises_.begin();
    while (it != promises_.end()) {
        auto* const promise = it->second;
        if (skip) {
            const auto s = skip->find(promise);
            if (s != skip->end()) {
                ++it;
                continue;
            }
        }
        if (!promise->isFinished()) {
            toAbort.insert(promise);

            // Don't abort promise while looping on promises
            // container: Aborting a promise emit the finished signal,
            // thus the promises container may be updated...
        }
        ++it;
    }
    for (auto* promise : toAbort)
        promise->abort();
}

void TileItemGroupHelper::identifyObsoleteTiles(
    const std::unordered_set<QVariant>& keepTiles,
    const std::list<std::pair<QRect, int>>& tileRects) {
    obsoleteTiles_.clear();

    auto it = std::begin(tiles_);
    const auto end = std::end(tiles_);
    while (it != end) {
        const auto s = keepTiles.find(it->first);
        if (s != std::end(keepTiles)) {
            ++it;
            continue;
        }
        bool inViewRect = false;
        for (auto& p : tileRects) {
            const auto rect = p.first;
            const auto level = p.second;
            int levelDepth = (source_->alwaysUpdated()) ? 0 : 2;
            inViewRect
                = source_->tileIntersect(it->first, rect, level, levelDepth);
            if (inViewRect)
                break;
        }
        if (inViewRect) {
            ++it;
            continue;
        }
        obsoleteTiles_.push_back(it->first);

        ++it;
    }
}

void TileItemGroupHelper::removeObsoleteTiles() {
    for (auto tileKey : obsoleteTiles_) { // it is a tile iterator
        if (tiles_[tileKey]) {
            delete tiles_[tileKey];
            tiles_[tileKey] = nullptr;
        }
        tiles_.erase(tileKey);
    }
    obsoleteTiles_.clear();
}

void TileItemGroupHelper::onFinishedPromise() {
    auto* const promise = static_cast<TilePromise*>(sender());
    auto* const item = promise->item();
    const auto tileKey = promise->tileKey();

    // When tileKey already exist, add it to obsolete before inserting
    // the new one
    auto it = tiles_.find(tileKey);
    if (it != tiles_.end()) {
        obsoleteTiles_.push_back(tileKey);
    }

    promises_.erase(tileKey);
    promise->deleteLater();
    removeObsoleteTiles();

    if (item) {
        tiles_.insert(std::make_pair(tileKey, item));
    }
}
