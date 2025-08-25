#pragma once

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

// TODO Check license rights; They depends on zoom level no?

struct MAPWIDGET_LIBRARY WorldImagerySource : NetworkTileSource {
    WorldImagerySource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY WorldStreetMapSource : NetworkTileSource {
    WorldStreetMapSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY WorldTopoMapSource : NetworkTileSource {
    WorldTopoMapSource(const QString& folderCacheName = "");
};
