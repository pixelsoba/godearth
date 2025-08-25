#include "mapwidget/tilesources/tmstilesource.h"

TMSTileSource::TMSTileSource(const QUrl& baseUrl, const QString& path,
                             const QString& name, int minZoomLevel,
                             int maxZoomLevel, int tileSize,
                             const QString& folderPathName,
                             const QStringList& subdomains,
                             const QString& legalString)
    : NetworkTileSource{baseUrl,        path,         name,
                        minZoomLevel,   maxZoomLevel, tileSize,
                        folderPathName, subdomains,   legalString} {}

TMSTileSource::TMSTileSource(const QString& baseUrlPath, const QString& path,
                             const QString& name, int minZoomLevel,
                             int maxZoomLevel, int tileSize,
                             const QString& folderPathName,
                             const QStringList& subdomains,
                             const QString& legalString)
    : NetworkTileSource{baseUrlPath,    path,         name,
                        minZoomLevel,   maxZoomLevel, tileSize,
                        folderPathName, subdomains,   legalString} {}

QUrl TMSTileSource::getUrl(const TileKeyType& key) const {
    const int yMax = 1 << key.z();
    const int y = yMax - key.y() - 1;

    QUrl url{baseUrl_};
    const auto path = url.path()
                      + tilePathTemplate_.arg(QString::number(key.z()),
                                  QString::number(key.x()), QString::number(y));
    url.setPath(path);
    if (subdomainsEnabled_ && !subdomains_.empty()) {
        static int count = 0;

        Q_ASSERT(0 <= count && count < subdomains_.size());
        const auto updated
            = QString{"%1.%2"}.arg(subdomains_.at(count), url.host());
        url.setHost(updated);

        count = (count + 1) % subdomains_.size();
    }
    return url;
}
