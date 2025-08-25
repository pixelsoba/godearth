#include "mapwidget/tilesources/vendors/osm.h"

namespace {
const QStringList subdomains{"a", "b", "c"};

const QString query;

const QString legal = "<a href=\"http://openstreetmap.org\">OpenStreetMap</a>";
}

OSMSource::OSMSource(const QString& folderCacheName)
    : NetworkTileSource{"https://tile.openstreetmap.org",
                        "/%1/%2/%3.png",
                        "OpenStreetMap",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        legal} {}

OCMSource::OCMSource(const QString& folderCacheName)
    : NetworkTileSource{"https://tile.opencyclemap.org/cycle",
                        "/%1/%2/%3.png",
                        "OpenCycleMap",
                        0,
                        18,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        legal} {}
