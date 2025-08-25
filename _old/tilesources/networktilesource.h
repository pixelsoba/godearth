#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QUrl>

#include "mapwidget/global.h"
#include "mapwidget/graphicsitems/tileitem.h"
#include "mapwidget/tilesources/tilepromise.h"
#include "mapwidget/tilesources/tilesourceimpl.h"
#include "mapwidget/tilesources/xyz.h"
#include "mapwidget/utilities/networkreplytimeout.h"

class MAPWIDGET_LIBRARY NetworkTilePromise : public TilePromise {

    Q_OBJECT

public:
    NetworkTilePromise(const QVariant& tileKey, TileItemGroup* group,
                       QNetworkReply* reply, QObject* parent = nullptr)
        : TilePromise{tileKey, group, parent}, reply_{reply} {
        if (reply_) {
            reply_->setParent(this);

            connect(reply_, &QNetworkReply::finished, this,
                    &NetworkTilePromise::finished);
        }
    }

    bool isFinished() const override {
        return reply_ ? reply_->isFinished() : true;
    }

    void abort() override {
        if (reply_)
            reply_->abort();
    }

    TileItem* item() const override {
        if (!reply_ || reply_->isRunning())
            return nullptr;

        if (reply_->error() != QNetworkReply::NoError)
            return nullptr;

        auto content = reply_->readAll();

        QPixmap p;
        p.loadFromData(content);

        if (p.isNull())
            return nullptr;

        auto* const t = new TileItem{group_, tileKey_};
        t->setPixmap(p);

        return t;
    }

private:
    QNetworkReply* const reply_;
};

template class TileSourceImpl<XYZ>;

class MAPWIDGET_LIBRARY NetworkTileSource : public TileSourceImpl<XYZ> {

    typedef TileSourceImpl<XYZ> BaseType;

public:
    NetworkTileSource(const QString& baseUrlPath,
                      const QString& tilePathTemplate, const QString& name,
                      int minZoomLevel = 0, int maxZoomLevel = 28,
                      int tileSize = 256, const QString& folderCacheName = "",
                      const QStringList& subdomains = {},
                      const QString& queryString = "",
                      const QString& legalString = "",
                      const QString& token = "")
        : BaseType{name,       minZoomLevel,    maxZoomLevel,
                   tileSize,   folderCacheName, tilePathTemplate,
                   legalString},
          baseUrl_{baseUrlPath},
          subdomains_{subdomains},
          subdomainsEnabled_{false},
          queryString_{queryString},
          token_{token} {

        if (folderCacheName.isEmpty()) {
            this->setFolderCacheName(name);
        }
    }

    NetworkTileSource(const QUrl& baseUrl, const QString& tilePathTemplate,
                      const QString& name, int minZoomLevel = 0,
                      int maxZoomLevel = 28, int tileSize = 256,
                      const QString& folderCacheName = "",
                      const QStringList& subdomains = {},
                      const QString& queryString = "",
                      const QString& legalString = "",
                      const QString& token = "")
        : BaseType{name,       minZoomLevel,    maxZoomLevel,
                   tileSize,   folderCacheName, tilePathTemplate,
                   legalString},
          baseUrl_{baseUrl},
          subdomains_{subdomains},
          subdomainsEnabled_{false},
          queryString_{queryString},
          token_{token} {

        if (folderCacheName.isEmpty()) {
            this->setFolderCacheName(name);
        }
    }

    QNetworkRequest getImageRequest(const QVariant& tileKey) const {

        QNetworkRequest req;
        const auto key = tileKey.value<TileKeyType>();
        if (key.valid()) {
            const QUrl url = getUrl(key);
            req.setUrl(url);
            req.setRawHeader("User-Agent", // required by some servers
                             "Delair map widget");
        }
        return req;
    }

    TilePromise* createTile(const QVariant& tileKey,
                            TileItemGroup* group) const override {

        auto* map = group ? group->map() : nullptr;
        if (!map)
            return nullptr;

        auto* const networkMgr = map->networkManager();
        if (!networkMgr)
            return nullptr;

        auto req = getImageRequest(tileKey);
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::PreferCache);

        auto* const reply = networkMgr->get(req);
        NetworkReplyTimeout::decorate(reply);

        return new NetworkTilePromise{tileKey, group, reply};
    }

    QString cachePath(const QUrl& url,
                      const QString& cacheRootPath) const override {

        QUrl baseUrl{baseUrl_};
        auto path = baseUrl_.path();
        if (!token_.isEmpty() && path.contains("$key$", Qt::CaseInsensitive)) {
            path.replace("$key$", token_);
            baseUrl.setPath(path);
        }
        if (!queryString_.isEmpty()) {
            auto query = queryString_;
            if (query.contains("%1") && query.contains("%2")
                && query.contains("%3")) {
                int index = std::min(query.indexOf("%1"), query.indexOf("%2"));
                index = std::min(index, query.indexOf("%3"));
                if (index > 0) {
                    query = query.left(index - 1);
                }
            }
            if (!token_.isEmpty()
                && query.contains("$key$", Qt::CaseInsensitive)) {
                query.replace("$key$", token_);
            }
            baseUrl.setQuery(query);
        }

        if (!subdomainsEnabled_) {
            if (!baseUrl.isParentOf(url)) {
                return QString{};
            }
            if (!url.query().contains(baseUrl.query())) {
                return QString{};
            }
        } else if (!url.host().endsWith(baseUrl.host())) {
            return QString{};
        }

        const auto& folderCacheName = this->folderCacheName();
        if (folderCacheName.isEmpty()) {
            return QString{};
        }

        auto basePath = baseUrl.path();
        if (basePath.isEmpty()) {
            basePath = "/";
        }

        const auto urlPath = url.path();
        Q_ASSERT(urlPath.startsWith(basePath));
        const auto relativePath = urlPath.mid(basePath.size());
        const QDir dir{cacheRootPath};
        return dir.absoluteFilePath(folderCacheName + "/" + relativePath);
    }

    int cacheDelay() const override {
        return 8640000; // 3600 * 24 * 100
    }

    bool subdomainsEnabled() const { return subdomainsEnabled_; }

    void setSubdomainsEnabled(bool enable) { subdomainsEnabled_ = enable; }

    QUrl getUrl(const QVariant& key) const {
        return getUrl(key.value<TileKeyType>());
    }

    QUrl baseUrl() const { return baseUrl_; }

    QString queryStr() const { return queryString_; }

    QString token() const { return token_; }

protected:
    const QUrl baseUrl_;

    const QStringList subdomains_;

    bool subdomainsEnabled_;

    const QString queryString_;

    const QString token_;

    virtual QUrl getUrl(const TileKeyType& key) const {

        QUrl url{baseUrl_};
        auto path = url.path();
        if (!tilePathTemplate_.isEmpty()) {
            path += tilePathTemplate_.arg(QString::number(key.z()),
                                          QString::number(key.x()),
                                          QString::number(key.y()));
        }
        if (!token_.isEmpty() && path.contains("$key$", Qt::CaseInsensitive)) {
            path.replace("$key$", token_);
        }

        url.setPath(path);
        if (subdomainsEnabled_ && !subdomains_.empty()) {
            static int count = 0;

            Q_ASSERT(0 <= count && count < subdomains_.size());
            const auto updated
                = QString{"%1.%2"}.arg(subdomains_.at(count), url.host());
            url.setHost(updated);

            count = (count + 1) % subdomains_.size();
        }

        if (!queryString_.isEmpty()) {
            auto query = queryString_;
            if (query.contains("%1") && query.contains("%2")
                && query.contains("%3")) {
                query = query.arg(QString::number(key.z()),
                                  QString::number(key.x()),
                                  QString::number(key.y()));
            }
            if (!token_.isEmpty()
                && query.contains("$key$", Qt::CaseInsensitive)) {
                query.replace("$key$", token_);
            }
            url.setQuery(query);
        }
        return url;
    }
};
