#include "mapwidget/tilesources/vendors/arcgisonline.h"

namespace {
const QStringList subdomains;

const QString query;
}

// Legal strings trimmed out (with an "et al." mention) for length reasons
// From: https://support.esri.com/en/technical-article/000012040

WorldImagerySource::WorldImagerySource(const QString& folderCacheName)
    : NetworkTileSource{"https://services.arcgisonline.com/ArcGIS/rest/services"
                        "/World_Imagery/MapServer/tile",
                        "/%1/%3/%2.png", "World Imagery",
                        0,
                        18,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        "ArcGIS Online, Esri, "
                        "DigitalGlobe, GeoEye, i-cubed, USDA FSA, USGS, AEX, "
                        "IGN, IGP, "
                        "<i>et al</i>."} {}

WorldStreetMapSource::WorldStreetMapSource(const QString& folderCacheName)
    : NetworkTileSource{"https://services.arcgisonline.com/ArcGIS/rest/services"
                        "/World_Street_Map/MapServer/tile",
                        "/%1/%3/%2.png", "World Street Map",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        "ArcGIS Online, Esri, DeLorme, "
                        "HERE, USGS, Intermap, iPC, NRCAN, METI, MapmyIndia, "
                        "Tomtom, "
                        "<i>et al</i>."} {}

WorldTopoMapSource::WorldTopoMapSource(const QString& folderCacheName)
    : NetworkTileSource{"https://services.arcgisonline.com/ArcGIS/rest/services"
                        "/World_Topo_Map/MapServer/tile",
                        "/%1/%3/%2.png", "World Topo Map",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        "ArcGIS Online, Esri, DeLorme, "
                        "HERE, TomTom, GEBCO, USGS, FAO, NPS, NRCAN, "
                        "GeoBase, IGN, "
                        "<i>et al</i>."} {}
