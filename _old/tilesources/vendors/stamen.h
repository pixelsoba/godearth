#pragma once

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

// TODO Check license rights and overload TileSource::createRequest()
// to ensure that request fulfill servers expectations

struct MAPWIDGET_LIBRARY StamenTonerSource : NetworkTileSource {
    StamenTonerSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY StamenWatercolorSource : NetworkTileSource {
    StamenWatercolorSource(const QString& folderCacheName = "");
};
