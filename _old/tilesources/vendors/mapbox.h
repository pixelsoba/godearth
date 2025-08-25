#pragma once

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

// TODO Check license rights and overload TileSource::createRequest()
// to ensure that request fulfill servers expectations

struct MAPWIDGET_LIBRARY MapboxSatelliteSource : NetworkTileSource {
    MapboxSatelliteSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY MapboxStreetsSource : NetworkTileSource {
    MapboxStreetsSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY MapboxStreetsSatelliteSource : NetworkTileSource {
    MapboxStreetsSatelliteSource(const QString& folderCacheName = "");
};
