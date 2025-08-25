#pragma once

#include "mapwidget/global.h"
#include "mapwidget/tilesources/networktilesource.h"

// TMS based tile layers
//
// TMS tile indexing is identical to XYZ indexing but the origin is
// south-west and not north-west. In this implementation, XYZ is used
// internally but a conversion is done before requesting a tile.
//
// cf. http://wiki.openstreetmap.org/wiki/TMS
class MAPWIDGET_LIBRARY TMSTileSource : public NetworkTileSource {
public:
    TMSTileSource(const QUrl& baseUrl, const QString& path, const QString& name,
                  int minZoomLevel = 0, int maxZoomLevel = 28,
                  int tileSize = 256, const QString& folderPathName = "",
                  const QStringList& subdomains = {},
                  const QString& legalString = "");

    TMSTileSource(const QString& baseUrlPath, const QString& path,
                  const QString& name, int minZoomLevel = 0,
                  int maxZoomLevel = 28, int tileSize = 256,
                  const QString& folderPathName = "",
                  const QStringList& subdomains = {},
                  const QString& legalString = "");

protected:
    QUrl getUrl(const TileKeyType& key) const override;
};
