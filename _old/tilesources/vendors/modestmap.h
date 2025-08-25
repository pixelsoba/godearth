#pragma once

#include <QString>

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

// TODO Check license rights and overload TileSource::createRequest()
// to ensure that request fulfill servers expectations

struct MAPWIDGET_LIBRARY ModestMapSource : NetworkTileSource {
    ModestMapSource(const QString& folderCacheName = "");
};
