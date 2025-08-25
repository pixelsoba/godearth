#pragma once

#include <QString>

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

struct MAPWIDGET_LIBRARY OSMSource : NetworkTileSource {
    OSMSource(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY OCMSource : NetworkTileSource {
    OCMSource(const QString& folderCacheName = "");
};
