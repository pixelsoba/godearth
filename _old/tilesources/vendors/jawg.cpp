#include "mapwidget/tilesources/vendors/jawg.h"

namespace {
const QString queryString = "api-key=MAPWIDGET_JAWG_TOKEN";

const QStringList subdomains;

const QString legal = "<a href=\"http://jawg.io\">Jawg</a>, "
                      "<a href=\"http://openstreetmap.org\">OpenStreetMap</a>";
}

JawgSource::JawgSource(const QString& folderCacheName)
    : NetworkTileSource{"https://tile.jawg.io",
                        "/%1/%2/%3.png",
                        "Jawg Street Tiles",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        queryString,
                        legal} {}

JawgSunnySource::JawgSunnySource(const QString& folderCacheName)
    : NetworkTileSource{"https://tile.jawg.io/sunny",
                        "/%1/%2/%3.png",
                        "Jawg Sunny Tiles",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        queryString,
                        legal} {}

JawgDarkSource::JawgDarkSource(const QString& folderCacheName)
    : NetworkTileSource{"https://tile.jawg.io/dark",
                        "/%1/%2/%3.png",
                        "Jawg Dark Tiles",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        queryString,
                        legal} {}

JawgLightSource::JawgLightSource(const QString& folderCacheName)
    : NetworkTileSource{"https://tile.jawg.io/light",
                        "/%1/%2/%3.png",
                        "Jawg Light Tiles",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        queryString,
                        legal} {}

JawgSatelliteSource::JawgSatelliteSource(const QString& folderCacheName)
    : NetworkTileSource{"https://tile.jawg.io/satellite",
                        "/%1/%2/%3.jpg",
                        "Jawg Satellite Tiles",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        queryString,
                        legal} {}
