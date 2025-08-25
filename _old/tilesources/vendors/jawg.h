#pragma once

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

// TODO Check license rights and overload TileSource::createRequest()
// to ensure that request fulfill servers expectations

struct MAPWIDGET_LIBRARY JawgSource : NetworkTileSource {
    JawgSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY JawgSunnySource : NetworkTileSource {
    JawgSunnySource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY JawgDarkSource : NetworkTileSource {
    JawgDarkSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY JawgLightSource : NetworkTileSource {
    JawgLightSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY JawgSatelliteSource : NetworkTileSource {
    JawgSatelliteSource(const QString& folderCacheName = "");
};
