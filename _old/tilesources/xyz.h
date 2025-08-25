#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <functional> // std::hash

#include <QDebug> // to define XYZ streaming operator
#include <QPoint>
#include <QPointF>
#include <QRect>

#include "mapwidget/compat.h"
#include "mapwidget/global.h"
#include "mapwidget/map.h"

// Helper class to generate slippy map tilenames
//
// cf. http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
struct MAPWIDGET_LIBRARY XYZ {
    // needed for QMetaType (thus QVariant) compatibility
    XYZ()
        : z_{-1},
          x_{-1},
          y_{-1} // invalid XYZ
    {}

    XYZ(int x, int y, int z) : z_{z}, x_{x}, y_{y} {}

    XYZ(const QPointF& scenePos, int z) : z_{z} {
        initializeXYComponents(scenePos);
    }

    bool operator==(const XYZ& rhs) const {
        return (z_ == rhs.z_ && x_ == rhs.x_ && y_ == rhs.y_);
    }

    bool operator!=(const XYZ& rhs) const { return !(this)->operator==(rhs); }

    bool operator<(const XYZ& rhs) const {
        // lexicographical order of zxy
        return (z_ < rhs.z_ || (z_ == rhs.z_ && x_ < rhs.x_)
                || (z_ == rhs.z_ && x_ == rhs.x_ && y_ < rhs.y_));
    }

    operator QPoint() const { return {x_, y_}; }

    bool valid() const {
        if (z_ < 0)
            return false;

        const auto size = std::pow(2, z_);
        return (0 <= x_ && x_ <= size) && (0 <= y_ && y_ <= size);
    }

    QPointF scenePos() const {
        const double t = M_PI - 2.0 * M_PI * y_ / std::pow(2, z_);
        const OGRPoint pos{x_ * 360.0 / std::pow(2, z_) - 180.0,
                           std::atan(std::sinh(t)) * 180.0 / M_PI};
        return Map::mapFromLonLat(pos);
    }

    QRectF sceneRect() const {
        const double t = M_PI - 2.0 * M_PI * y_ / std::pow(2, z_);
        const OGRPoint topLeft{x_ * 360.0 / std::pow(2, z_) - 180.0,
                               std::atan(std::sinh(t)) * 180.0 / M_PI};
        const double otherT = M_PI - 2.0 * M_PI * (y_ + 1) / std::pow(2, z_);
        const OGRPoint bottomRight{(x_ + 1) * 360.0 / std::pow(2, z_) - 180.0,
                                   std::atan(std::sinh(otherT)) * 180.0 / M_PI};
        return {Map::mapFromLonLat(topLeft), Map::mapFromLonLat(bottomRight)};
    }

    XYZ parent() const { return {x_ / 2, y_ / 2, z_ - 1}; }

    std::array<XYZ, 4> children() const {
        const std::array<XYZ, 4> tiles{XYZ{2 * x_, 2 * y_, z_ + 1},
                                       XYZ{2 * x_ + 1, 2 * y_, z_ + 1},
                                       XYZ{2 * x_, 2 * y_ + 1, z_ + 1},
                                       XYZ{2 * x_ + 1, 2 * y_ + 1, z_ + 1}};
        return tiles;
    }

    static QRect extent(int z) {
        const int size = std::pow(2, z) - 1;
        return {0, 0, size, size};
    }

    int x() const { return x_; }

    int y() const { return y_; }

    int z() const { return z_; }

private:
    int z_, x_, y_;

    void initializeXYComponents(const QPointF& scenePos) {
        const auto p = Map::mapToLonLat(scenePos);
        const double lon = p.getX();
        const double lat = p.getY();

        const double latrad = lat * M_PI / 180.0;
        x_ = static_cast<int>(
            std::floor(std::pow(2, z_) * ((lon + 180.0) / 360.0)));
        y_ = static_cast<int>(std::floor(
            std::pow(2, z_)
            * (1.0
               - (std::log(std::tan(latrad) + 1.0 / std::cos(latrad)) / M_PI))
            / 2));
    }
};

Q_DECLARE_METATYPE(XYZ)

namespace std {
template <> struct hash<XYZ> {
    typedef XYZ argument_type;
    typedef uint64_t result_type;

    result_type operator()(argument_type const& xyz) const {
        // no collision for z < 28 and valid xyz
        const result_type hx{static_cast<result_type>(xyz.x()) << (64 - 28)};
        const result_type hy{static_cast<result_type>(xyz.y()) << (64 - 28)
                             >> 28};
        const result_type hz{(static_cast<result_type>(xyz.z()) << (64 - 8))
                             >> (64 - 8)};
        return hx ^ hy ^ hz;
    }
};
}

inline QDebug operator<<(QDebug debug, const XYZ& xyz) {
    QDebugStateSaver saver{debug};
    debug.nospace() << xyz.z() << " " << xyz.x() << " " << xyz.y();
    return debug;
}
