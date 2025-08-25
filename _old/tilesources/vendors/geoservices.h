#pragma once

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

struct MAPWIDGET_LIBRARY GoeServicesOACI : NetworkTileSource {
    GoeServicesOACI(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY GoeServicesUasRestrictions : NetworkTileSource {
    GoeServicesUasRestrictions(const QString& folderCacheName = "");
};

struct MAPWIDGET_LIBRARY GoeServicesPopulationDensity : NetworkTileSource {
    GoeServicesPopulationDensity(const QString& folderCacheName = "");
};
