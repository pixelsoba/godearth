#include "mapwidget/tilesources/vendors/mapbox.h"

#include "mapwidget/credentials.h"

namespace {
const QString query = QString("access_token=$key$");

const QStringList subdomains;

const QString legal = "<a href=\"http://mapbox.com\">Mapbox</a>, "
                      "<a href=\"http://openstreetmap.org\">OpenStreetMap</a>";
} // namespace

MapboxSatelliteSource::MapboxSatelliteSource(const QString& folderCacheName)
    : NetworkTileSource{
        "https://api.mapbox.com/styles/v1/mapbox/satellite-v9/tiles",
        "/%1/%2/%3",
        "Mapbox Satellite Tiles",
        0,
        19,
        256,
        folderCacheName,
        subdomains,
        query,
        legal,
        MAPWIDGET_MAPBOX_TOKEN} {}

MapboxStreetsSource::MapboxStreetsSource(const QString& folderCacheName)
    : NetworkTileSource{
        "https://api.mapbox.com/styles/v1/mapbox/streets-v11/tiles",
        "/%1/%2/%3",
        "Mapbox Streets Tiles",
        0,
        19,
        256,
        folderCacheName,
        subdomains,
        query,
        legal,
        MAPWIDGET_MAPBOX_TOKEN} {}

MapboxStreetsSatelliteSource::MapboxStreetsSatelliteSource(
    const QString& folderCacheName)
    : NetworkTileSource{"https://api.mapbox.com/styles/v1/mapbox/"
                        "satellite-streets-v11/tiles",
                        "/%1/%2/%3",
                        "Mapbox Mixed Tiles",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        legal,
                        MAPWIDGET_MAPBOX_TOKEN} {}
