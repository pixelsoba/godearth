#include "mapwidget/tilesources/vendors/modestmap.h"

namespace {
const QStringList subdomains;

const QString query;

const QString legal
    = "<a href=\"http://modestmaps.com\">Modest Maps</a>, "
      "<a href=\"https://earthobservatory.nasa.gov\">NASA Earth "
      "Observatory</a>";
}

ModestMapSource::ModestMapSource(const QString& folderCacheName)
    : NetworkTileSource{"http://s3.amazonaws.com/com.modestmaps.bluemarble",
                        "/%1-r%3-c%2.jpg",
                        "Blue Marble",
                        0,
                        19,
                        256,
                        folderCacheName,
                        subdomains,
                        query,
                        legal} {}
