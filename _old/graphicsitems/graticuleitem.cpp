#include "mapwidget/graphicsitems/graticuleitem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

#include <QGraphicsView>

#include "mapwidget/graphicsitems/common.h"
#include "mapwidget/map.h"
#include "mapwidget/views/mapview.h"

namespace {
std::pair<double, double> level2steps(int level) {
    double step = 100;
    if (level < 5)
        step = 45;
    else if (5 <= level && level < 8)
        step = 10;
    else if (8 <= level && level < 11)
        step = 1;
    else if (11 <= level && level < 15)
        step = .1;
    else
        step = .01;

    return {step, step};
}
} // namespace

GraticuleItem::GraticuleItem(QGraphicsItem* parent)
    : QGraphicsItem{parent},
      unitPen_{Qt::blue},
      decimalPen_{Qt::blue},
      centesimalPen_{Qt::lightGray},
      unitBrush_{Qt::blue},
      decimalBrush_{Qt::blue},
      centesimalBrush_{Qt::lightGray} {
    setData(CustomQt::UserCantRemove, true);
    setData(CustomQt::UserCantEditGeometry, true);
    setData(CustomQt::ItemNameKey, QObject::tr("Graticule"));

    unitPen_.setCosmetic(true);
    decimalPen_.setCosmetic(true);
    centesimalPen_.setCosmetic(true);

    unitPen_.setStyle(Qt::SolidLine);
    decimalPen_.setStyle(Qt::DashLine);
    centesimalPen_.setStyle(Qt::DashLine);

    font_.setStyleHint(QFont::SansSerif);
    font_.setStyleStrategy(QFont::ForceOutline); // better for rescaling
    font_.setPointSizeF(8.0);                    // at small size
}

QRectF GraticuleItem::boundingRect() const {
    auto* const s = scene();
    QRectF sceneRect = s ? s->sceneRect() : QRectF{};
    return {-sceneRect.width() / 2, sceneRect.height() / 2, sceneRect.width(),
            -sceneRect.height()};
}

void GraticuleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*,
                          QWidget* widget) {
    auto* const view = qobject_cast<MapView*>(widget->parent());
    if (!view)
        return;

    auto* const map = view->map();
    if (!map)
        return;

    // TODO: The graticule grid computation here only works for map projections
    // that preserve lines (which is the case for EPSG:3857 pseudo-mecator). The
    // label positionning also assume the middle of drawn grid lines to be at
    // the center of the view.

    const QRect rect = view->rect();
    const QPolygonF sceneRect = view->mapToScene(rect);
    const QPolygonF clipped = sceneRect.intersected(map->scene()->sceneRect());

    // For labels to be at the correct position and with constant scale:
    double scale
        = 1.0 / (std::sqrt(std::fabs(view->transform().determinant())));
    QTransform tLocal;
    tLocal.scale(scale, -scale); // y-axis inverted on map
    tLocal.translate(5.0, -5.0); // small offset from the grid lines

    std::vector<double> lon;
    std::vector<double> lat;
    lon.reserve(static_cast<size_t>(clipped.size()));
    lat.reserve(static_cast<size_t>(clipped.size()));
    for (auto& point : clipped) {
        const auto pos = Map::mapToLonLat(point);
        lon.emplace_back(pos.getX());
        lat.emplace_back(pos.getY());
    }

    double leftLon = *std::min_element(std::begin(lon), std::end(lon));
    double rightLon = *std::max_element(std::begin(lon), std::end(lon));
    double topLat = *std::max_element(std::begin(lat), std::end(lat));
    double bottomLat = *std::min_element(std::begin(lat), std::end(lat));

    leftLon = std::max(Map::lonMin_, leftLon);
    rightLon = std::min(Map::lonMax_, rightLon);
    topLat = std::min(Map::latMax_, topLat);
    bottomLat = std::max(Map::latMin_, bottomLat);

    const double res = view->resolution();
    const int level = view->level(res);
    const std::pair<double, double> steps = level2steps(level);

    QPainterPath unitPath;
    QPainterPath decimalPath;
    QPainterPath centesimalPath;
    QPainterPath* path = nullptr;

    QPainterPath unitLabelPath;
    QPainterPath decimalLabelPath;
    QPainterPath centesimalLabelPath;
    QPainterPath* labelPath = nullptr;
    int prec;

    const double meridianStart
        = std::floor(leftLon / steps.first) * steps.first;
    const double meridianStop = std::ceil(rightLon / steps.first) * steps.first;
    for (double i = meridianStart; i <= meridianStop; i += steps.first) {
        if (std::abs(i - std::round(i)) < .00001) {
            path = &unitPath;
            labelPath = &unitLabelPath;
            prec = 0;
        } else if (std::abs(i - std::round(i * 10) / 10) < .00001) {
            path = &decimalPath;
            labelPath = &decimalLabelPath;
            prec = 1;
        } else {
            path = &centesimalPath;
            labelPath = &centesimalLabelPath;
            prec = 2;
        }

        Q_ASSERT(path);
        const auto start = Map::mapFromLonLat(i, topLat);
        path->moveTo(start);
        const auto stop = Map::mapFromLonLat(i, bottomLat);
        path->lineTo(stop);

        if (std::abs(i) < 180.0) {
            // All labels except the 180W and 180E that would be at the map
            // edges.
            QPainterPath p;
            p.addText({0, 0}, font_,
                      QString::number(std::abs(i), 'f', prec) + "°"
                          + ((i < 0) ? QObject::tr("W") : QObject::tr("E")));
            p = tLocal.map(p);
            QPointF px{start.x(), (start.y() + stop.y()) / 2};
            p.translate(px);
            labelPath->addPath(p);
        }
    }

    const double parallelStart
        = std::floor(bottomLat / steps.second) * steps.second;
    const double parallelStop = std::ceil(topLat / steps.second) * steps.second;
    for (double i = parallelStart; i <= parallelStop; i += steps.second) {
        if (std::abs(i - std::round(i)) < .00001) {
            path = &unitPath;
            labelPath = &unitLabelPath;
            prec = 0;
        } else if (std::abs(i - std::round(i * 10) / 10) < .00001) {
            path = &decimalPath;
            labelPath = &decimalLabelPath;
            prec = 1;
        } else {
            path = &centesimalPath;
            labelPath = &centesimalLabelPath;
            prec = 2;
        }

        Q_ASSERT(path);
        const auto start = Map::mapFromLonLat(leftLon, i);
        path->moveTo(start);
        const auto stop = Map::mapFromLonLat(rightLon, i);
        path->lineTo(stop);

        if (std::abs(i) < 90.0) {
            // All labels except the 90N and 90S that would be at the map edges.
            QPainterPath p;
            p.addText({0, 0}, font_,
                      QString::number(std::abs(i), 'f', prec) + "°"
                          + ((i < 0) ? QObject::tr("S") : QObject::tr("N")));
            p = tLocal.map(p);
            QPointF px{(start.x() + stop.x()) / 2, start.y()};
            p.translate(px);
            labelPath->addPath(p);
        }
    }

    if (!unitPath.isEmpty()) {
        painter->save();
        painter->setPen(unitPen_);
        painter->drawPath(unitPath);

        painter->setBrush(unitBrush_);
        painter->setPen(Qt::NoPen); // No outline (ugly at some zoom level).
        painter->drawPath(unitLabelPath);
        painter->restore();
    }
    if (!decimalPath.isEmpty()) {
        painter->save();
        painter->setPen(decimalPen_);
        painter->drawPath(decimalPath);

        painter->setBrush(decimalBrush_);
        painter->setPen(Qt::NoPen); // No outline (ugly at some zoom level).
        painter->drawPath(decimalLabelPath);
        painter->restore();
    }
    if (!centesimalPath.isEmpty()) {
        painter->save();
        painter->setPen(centesimalPen_);
        painter->drawPath(centesimalPath);

        painter->setBrush(centesimalBrush_);
        painter->setPen(Qt::NoPen); // No outline (ugly at some zoom level).
        painter->drawPath(centesimalLabelPath);
        painter->restore();
    }
}

void GraticuleItem::setUnitPen(const QPen& pen) {
    if (!pen.isCosmetic())
        return;

    unitPen_ = pen;
}

void GraticuleItem::setDecimalPen(const QPen& pen) {
    if (!pen.isCosmetic())
        return;

    decimalPen_ = pen;
}

void GraticuleItem::setCentesimalPen(const QPen& pen) {
    if (!pen.isCosmetic())
        return;

    centesimalPen_ = pen;
}

void GraticuleItem::setUnitBrush(const QBrush& brush) { unitBrush_ = brush; }

void GraticuleItem::setDecimalBrush(const QBrush& brush) {
    decimalBrush_ = brush;
}

void GraticuleItem::setCentesimalBrush(const QBrush& brush) {
    centesimalBrush_ = brush;
}
