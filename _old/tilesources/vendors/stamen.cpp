#include "mapwidget/tilesources/vendors/stamen.h"

namespace {
const QStringList subdomains;

const QString query;

const QString legal
    = "<a href=\"http://stamen.com\">Stamen Design</a>, "
      "<a href=\"http://openstreetmap.org\">OpenStreetMap</a>";
}

StamenTonerSource::StamenTonerSource(const QString& folderCacheName)
    : NetworkTileSource{"http://tile.stamen.com/toner",
                        "/%1/%2/%3.png",
                        "Stamen Toner",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        legal} {}

StamenWatercolorSource::StamenWatercolorSource(const QString& folderCacheName)
    : NetworkTileSource{"http://tile.stamen.com/watercolor",
                        "/%1/%2/%3.jpg",
                        "Stamen Watercolor",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        legal} {}
