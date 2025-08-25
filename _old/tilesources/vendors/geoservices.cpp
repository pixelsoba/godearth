#include "mapwidget/tilesources/vendors/geoservices.h"

#include "mapwidget/credentials.h"

namespace {
const QStringList subdomains;
}

GoeServicesOACI::GoeServicesOACI(const QString& folderCacheName)
    : NetworkTileSource{
        "https://wxs.ign.fr/$key$/geoportail/wmts",
        "/%1/%2/%3",
        "OACI (France)",
        7,
        11,
        256,
        folderCacheName,
        subdomains,
        "layer=GEOGRAPHICALGRIDSYSTEMS.MAPS.SCAN-OACI&style=normal&"
        "tilematrixset=PM&Service=WMTS&Request=GetTile&Version=1.0.0&Format="
        "image/jpeg&TileMatrix=%1&TileCol=%2&TileRow=%3",
        "",
        GEOSERVICES_TOKEN} {}

GoeServicesUasRestrictions::GoeServicesUasRestrictions(
    const QString& folderCacheName)
    : NetworkTileSource{
        "https://wxs.ign.fr/transports/geoportail/wmts",
        "/%1/%2/%3",
        "UAS Restrictions (France)",
        7,
        15,
        256,
        folderCacheName,
        subdomains,
        "layer=TRANSPORTS.DRONES.RESTRICTIONS&style=normal&tilematrixset=PM&"
        "Service=WMTS&Request=GetTile&Version=1.0.0&Format=image/png&"
        "TileMatrix=%1&TileCol=%2&TileRow=%3"} {}

GoeServicesPopulationDensity::GoeServicesPopulationDensity(
    const QString& folderCacheName)
    : NetworkTileSource{
        "https://wxs.ign.fr/economie/geoportail/wmts",
        "/%1/%2/%3",
        "Population Density (France)",
        8,
        15,
        256,
        folderCacheName,
        subdomains,
        "layer=INSEE.FILOSOFI.POPULATION&style=INSEE&tilematrixset=PM&Service="
        "WMTS&Request=GetTile&Version=1.0.0&Format=image/png&"
        "TileMatrix=%1&TileCol=%2&TileRow=%3"} {}
