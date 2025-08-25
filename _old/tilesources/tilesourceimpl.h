#pragma once

#include <QDir>
#include <QString>
#include <QUrl>

#include "mapwidget/tilesources/tilesource.h"

#include "mapwidget/graphicsitems/tileitem.h"
#include "mapwidget/graphicsitems/tileitemgroup.h"

// Partial implementation of tile source for XYZ-like pyramids.
//
// TileKeyType must be convertible to/from QVariant and it must have a
// std::hash specialization.
template <typename T> class TileSourceImpl : public TileSource {
public:
    typedef T TileKeyType;

    TileSourceImpl(const QString& name, int minZoomLevel = 0,
                   int maxZoomLevel = 28, int tileSize = 256,
                   const QString& folderCacheName = "",
                   const QString& tilePathTemplate = "",
                   const QString& legalString = "", bool alwaysUpdated = false)
        : TileSource{name,        minZoomLevel,    maxZoomLevel,
                     tileSize,    folderCacheName, tilePathTemplate,
                     legalString, alwaysUpdated} {}

    QVariant tileKey(const QPointF& scenePos, int level,
                     bool* valid = nullptr) const override {
        const TileKeyType key{scenePos, level};
        if (valid)
            *valid = key.valid();

        return QVariant::fromValue(key);
    }

    QVariant tileKey(const QPoint& coord, // in given level plane
                     int level, bool* valid = nullptr) const override {
        const TileKeyType key{coord.x(), coord.y(), level};
        if (valid)
            *valid = key.valid();

        return QVariant::fromValue(key);
    }

    QPoint tileCoord(const QPointF& scenePos, int level,
                     bool* valid = nullptr) const override {
        const TileKeyType key{scenePos, level};
        if (valid)
            *valid = key.valid();
        return key;
    }

    QPoint tileCoord(const QVariant& tileKey,
                     bool* valid = nullptr) const override {
        const auto key = tileKey.value<TileKeyType>();
        if (valid)
            *valid = key.valid();
        return key;
    }

    QVariantList childTileKeys(const QVariant& tileKey) const override {
        QVariantList children;
        const auto key = tileKey.value<TileKeyType>();
        if (key.valid()) {
            for (auto& child : key.children())
                children << QVariant::fromValue(child);
        }
        return children;
    }

    QVariant parentTileKey(const QVariant& tileKey) const override {
        const auto key = tileKey.value<TileKeyType>();
        if (key.valid()) {
            return QVariant::fromValue(key.parent());
        }
        return QVariant{};
    }

    bool tileIntersect(const QVariant& tileKey, const QRect& tileRect,
                       int level, int levelDepth = -1) const override {
        const auto key = tileKey.value<TileKeyType>();
        if (levelDepth != -1 && std::abs(key.z() - level) > levelDepth)
            return false;

        if (key.z() > level) {
            const int factor = std::pow(2, key.z() - level);
            const TileKeyType ancestorKey{key.x() / factor, key.y() / factor,
                                          level};
            return tileRect.contains(ancestorKey);
        }

        if (key.z() < level) {
            const int factor = std::pow(2, level - key.z());
            const auto topLeft = tileRect.topLeft();
            const TileKeyType topLeftAncestorKey{topLeft.x() / factor,
                                                 topLeft.y() / factor, key.z()};
            const auto bottomRight = tileRect.bottomRight();
            const TileKeyType bottomRightAncestorKey{
                bottomRight.x() / factor, bottomRight.y() / factor, key.z()};
            const QRect ancestorRect{topLeftAncestorKey,
                                     bottomRightAncestorKey};
            return ancestorRect.contains(key);
        }

        return tileRect.contains(key);
    }

    QRect extent(int level) const override {
        return TileKeyType::extent(level);
    }
};
