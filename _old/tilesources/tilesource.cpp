#include "mapwidget/tilesources/tilesource.h"

#include "mapwidget/graphicsitems/tileitem.h"

TileSource::TileSource(const QString& name, int minZoomLevel, int maxZoomLevel,
                       int tileSize, const QString& folderCacheName,
                       const QString& tilePathTemplate,
                       const QString& legalString, bool alwaysUpdated)
    : name_{name},
      folderCacheName_{folderCacheName},
      tilePathTemplate_{tilePathTemplate},
      legalString_{legalString},
      tileSize_{tileSize},
      minZoomLevel_{minZoomLevel},
      maxZoomLevel_{maxZoomLevel},
      alwaysUpdated_{alwaysUpdated} {

    Q_ASSERT(0 <= minZoomLevel);
    Q_ASSERT(minZoomLevel <= maxZoomLevel);
    Q_ASSERT(maxZoomLevel <= 28);

    this->populateResolutions(minZoomLevel, maxZoomLevel);
    this->populateLevels();

    Q_ASSERT(1 <= resolutions_.size());
}

void TileSource::populateResolutions(int minZoomLevel, int maxZoomLevel) {

    for (int l = minZoomLevel; l <= maxZoomLevel; ++l) {
        resolutions_[l] = Map::earthPerimeter_ / tileSize_ / std::pow(2, l);
    }
}

void TileSource::populateLevels() {

    for (auto p : resolutions_) {
        levels_.insert(std::make_pair(p.second, p.first));
    }
}

int TileSource::level(qreal resolution) const {

    const auto b = levels_.lower_bound(resolution);
    if (b == levels_.end()) {
        return levels_.rbegin()->second;
    }
    return b->second;
}

qreal TileSource::resolution(int level) const {

    level = std::min(level, maxZoomLevel_);
    level = std::max(level, minZoomLevel_);
    Q_ASSERT(resolutions_.find(level) != resolutions_.end());

    return resolutions_.at(level); // won't throw...
}

void TileSource::setMinMaxZoomLevel(int zoomMin, int zoomMax) {

    if (minZoomLevel_ == zoomMin && maxZoomLevel_ == zoomMax) {
        return;
    }

    minZoomLevel_ = zoomMin;
    maxZoomLevel_ = zoomMax;

    resolutions_.clear();
    levels_.clear();

    this->populateResolutions(zoomMin, zoomMax);
    this->populateLevels();

    Q_ASSERT(1 <= resolutions_.size());
}
