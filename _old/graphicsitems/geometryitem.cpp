#include "mapwidget/graphicsitems/geometryitem.h"

#include <cmath>
#include <memory>

#include <ogr_geometry.h>

#include <QApplication>
#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMap>
#include <QOpenGLWidget>
#include <QPainter>
#include <QWidget>

#include "mapwidget/compat.h"
#include "mapwidget/controls/measurecontrol.h"
#include "mapwidget/graphicsitems/common.h"
#include "mapwidget/map.h"
#include "mapwidget/models/foldermodel.h"
#include "mapwidget/utilities/gdal.h"
#include "mapwidget/utilities/sphericalgeom.h"

namespace {
const int cosmeticPenOutlineWidth{10};

template <typename T> QString styleComponentKey();

template <> QString styleComponentKey<QPen>() { return QStringLiteral("pen"); }

template <> QString styleComponentKey<QBrush>() {
    return QStringLiteral("brush");
}

template <> QString styleComponentKey<QFont>() {
    return QStringLiteral("font");
}

// TODO can be slow... Better store pointers to styles and
// invalidate children on style changes

template <typename T>
T resolveStyleComponent(const QGraphicsItem* item, const QString& id,
                        bool* found = nullptr) {
    const auto key = styleComponentKey<T>();
    while (item) {
        const auto styleMap = item->data(CustomQt::ItemStyleMap).toMap();
        const auto s = styleMap.find(id);
        if (s != styleMap.end()) {
            const auto style = s->toMap();
            const auto t = style.find(key);
            if (t != style.end()) {
                if (found)
                    *found = true;

                return qvariant_cast<T>(*t);
            }
        }
        item = item->parentItem();
    }
    if (found)
        *found = false;

    return T{};
}

QVariant resolveStyleComponent(const QGraphicsItem* item, const QString& id,
                               const QString& key, bool* found = nullptr) {
    while (item) {
        const auto styleMap = item->data(CustomQt::ItemStyleMap).toMap();
        const auto s = styleMap.find(id);
        if (s != styleMap.end()) {
            const auto style = s->toMap();
            const auto t = style.find(key);
            if (t != style.end()) {
                if (found)
                    *found = true;

                return *t;
            }
        }
        item = item->parentItem();
    }
    if (found)
        *found = false;

    return QVariant{};
}

const double defaultPointRadius{5};

const QString defaultPointShape{"circle"};
} // namespace

class RoundedSimpleTextItem : public QGraphicsSimpleTextItem {
public:
    RoundedSimpleTextItem(const QString& text, QGraphicsItem* parent = nullptr)
        : QGraphicsSimpleTextItem(text, parent),
          marginHorizontal_{5},
          marginVertical_{5} {}

    void setMargins(int horizontal, int vertical) {
        marginHorizontal_ = horizontal;
        marginVertical_ = vertical;
    }

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*,
               QWidget*) override;

public:
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

private:
    int marginHorizontal_;
    int marginVertical_;
};

QRectF RoundedSimpleTextItem::boundingRect() const {
    QRectF rect = QGraphicsSimpleTextItem::boundingRect();
    QRectF bgRect = rect.adjusted(-marginHorizontal_, -marginVertical_,
                                  marginHorizontal_, marginVertical_);

    bgRect.setWidth(qMax(bgRect.width(), bgRect.height()));

    qreal w = bgRect.width() / 2.0;
    qreal h = bgRect.height() / 2.0;
    bgRect.setTopLeft(QPointF{-w, -h});
    bgRect.setBottomRight(QPointF{w, h});

    return bgRect;
}

QPainterPath RoundedSimpleTextItem::shape() const {
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void RoundedSimpleTextItem::paint(QPainter* painter,
                                  const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setBrush(brush());
    painter->setPen(Qt::NoPen);
    QRectF bgRect = boundingRect();

    painter->drawRoundedRect(bgRect, bgRect.height() / 2, bgRect.height() / 2);

    painter->setPen(pen());
    painter->setFont(font());
    painter->drawText(bgRect, Qt::AlignCenter, text());
}

GeometryItem::GeometryItem(const OGRGeometry* geom, QGraphicsItem* parent)
    : QGraphicsObject{parent},
      geom_{geom ? geom->clone() : nullptr},
      currentStyle_{""},
      label_{nullptr} {
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setData(CustomQt::FolderModelBehaviorKey, FolderModel::ExcludeChildItems);
    setData(CustomQt::ItemCanBeSaved, true);
    setBoundingRegionGranularity(1);
}

GeometryItem::~GeometryItem() {
    if (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    delete geom_;
}

OGRGeometry* GeometryItem::geometry(OGRSpatialReference* srs) const {
    if (!geom_)
        return nullptr;

    auto* const geom = geom_->clone();

    auto* const sceneSrs = Map::spatialReference();
    if (srs && !srs->IsSame(sceneSrs)) {
        geom->transformTo(srs);
    }
    return geom;
}

void GeometryItem::setGeometry(const OGRGeometry* geom,
                               OGRSpatialReference* srs) {

    if (geom_) {
        delete geom_;
        geom_ = nullptr;
    }

    if (geom) {
        geom_ = geom->clone();
        geom_->assignSpatialReference(srs);
        auto* const sceneSrs = Map::spatialReference();
        if (srs && !srs->IsSame(sceneSrs)) {
            geom_->transformTo(sceneSrs);
        } else {
            auto itemSrs = geom_->getSpatialReference();
            if (itemSrs && itemSrs->IsSame(sceneSrs)) {
                geom_->transformTo(sceneSrs);
            } else if (!itemSrs) {
                geom_->assignSpatialReference(sceneSrs);
            }
        }
    }

    if (isEdited()) {
        updateItemFromGeometry();
    }

    updatePath(true);
}

QString GeometryItem::currentStyle() const { return currentStyle_; }

void GeometryItem::setCurrentStyle(const QString& id) {
    if (currentStyle_ == id)
        return;

    currentStyle_ = id;

    styleHasChanged();
}

GeometryItem::StyleMap GeometryItem::styleMap() const {
    const auto rawStyles = data(CustomQt::ItemStyleMap).toMap();
    StyleMap styleMap;
    for (auto it = rawStyles.begin(); it != rawStyles.end(); ++it) {
        const auto& id = it.key();
        const auto& style = it.value();
        styleMap[id] = style.toMap();
    }
    return styleMap;
}

void GeometryItem::setStyleMap(const StyleMap& styleMap) {
    QVariantMap rawStyles;
    for (auto it = styleMap.begin(); it != styleMap.end(); ++it) {
        const auto& id = it.key();
        const auto& style = it.value();
        rawStyles[id] = style;
    }
    setData(CustomQt::ItemStyleMap, rawStyles);

    styleHasChanged();
}

QPen GeometryItem::pen() const {
    auto p = resolveStyleComponent<QPen>(this, currentStyle_);
    return p;
}

QBrush GeometryItem::brush() const {
    return resolveStyleComponent<QBrush>(this, currentStyle_);
}

QFont GeometryItem::font() const {
    return resolveStyleComponent<QFont>(this, currentStyle_);
}

QVariant GeometryItem::resolveCurrentStyleComponent(const QString& key) const {
    return resolveStyleComponent(this, currentStyle_, key);
}

QPainterPath GeometryItem::symbolPath() const {
    const QVariant rawShape = resolveCurrentStyleComponent("point-shape");
    QString shape = rawShape.toString();

    if (shape != "circle" && shape != "square" && shape != "triangle"
        && shape != "cross" && shape != "diagcross") {
        shape = defaultPointShape;
    }
    const QVariant rawRadius = resolveCurrentStyleComponent("point-radius");
    double radius = rawRadius.toDouble();
    if (radius <= 0) {
        radius = defaultPointRadius;
    }
    const QRectF rect{QPointF{-radius, -radius}, QPointF{radius, radius}};
    QPainterPath path;
    if (shape == "circle") {
        path.addEllipse(rect);
    } else if (shape == "square") {
        path.addRect(rect);
    } else if (shape == "triangle") {
        path.moveTo(QPointF{0, -radius});
        path.lineTo(QPointF{std::sqrt(3) * radius / 2, radius / 2});
        path.lineTo(QPointF{-std::sqrt(3) * radius / 2, radius / 2});
        path.lineTo(QPointF{0, -radius});
    } else if (shape == "cross") {
        path.moveTo(QPointF{0, -radius});
        path.lineTo(QPointF{0, radius});
        path.moveTo(QPointF{-radius, 0});
        path.lineTo(QPointF{radius, 0});
    } else if (shape == "diagcross") {
        path.moveTo(QPointF{-radius, -radius});
        path.lineTo(QPointF{radius, radius});
        path.moveTo(QPointF{-radius, radius});
        path.lineTo(QPointF{radius, -radius});
    }
    return path;
}

QGraphicsSimpleTextItem* GeometryItem::label() const { return label_; }

QString GeometryItem::labelText() const {
    return label_ ? label_->text() : QString{};
}

void GeometryItem::setLabelText(const QString& text) {
    if (text.isEmpty()) {
        delete label_;
        label_ = nullptr;
    } else {
        if (!label_) {
            label_ = new RoundedSimpleTextItem{text, this};
            label_->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            setLabelStyle();
            updateLabelPosition();
        } else {
            label_->setText(text);
        }
    }
}

void GeometryItem::clearLabel() {
    delete label_;
    label_ = nullptr;
}

bool GeometryItem::labelIsVisible() const {
    return label_ ? label_->isVisible() : false;
}

void GeometryItem::setLabelVisible(bool visible) {
    if (!label_)
        return;

    label_->setVisible(visible);
}

void GeometryItem::updatePath(bool) { emit pathUpdated(); }

void GeometryItem::editedChanged(bool edited) {
    setCurrentStyle(edited ? "edited" : "");

    if (edited) {
        updateItemFromGeometry();
    } else {
        updateGeometryFromItem();
    }
    updatePath();
    emit editingChanged(edited);
}

void GeometryItem::styleHasChanged() {
    update();

    setLabelStyle();
    for (auto* childItem : childItems())
        childItem->update();
}

void GeometryItem::setLabelStyle() {
    if (!label_)
        return;

    bool found = false;
    const auto pen = resolveStyleComponent<QPen>(this, "label", &found);
    if (found) {
        label_->setPen(pen); // isn't it slow to use a pen?
    } else {
        label_->setPen(QPen(Qt::black));
    }

    const auto brush = resolveStyleComponent<QBrush>(this, "label", &found);
    if (found) {
        label_->setBrush(brush);
    } else {
        label_->setBrush(Qt::NoBrush);
    }

    const auto font = resolveStyleComponent<QFont>(this, "label", &found);
    if (found)
        label_->setFont(font);

    auto offsetPos
        = resolveStyleComponent(this, "label", "offset", &found).toPoint();
    if (!found) {
        offsetPos = QPoint{15, 15};
    }

    if (pos() != QPoint{0, 0}) {
        label_->setPos(offsetPos);
    }

    auto* roundedLabel = dynamic_cast<RoundedSimpleTextItem*>(label_);
    if (roundedLabel) {
        auto margins
            = resolveStyleComponent(this, "label", "margins", &found).toPoint();
        if (!found) {
            margins = QPoint(5, 5);
        }
        roundedLabel->setMargins(margins.x(), margins.y());
    }
}

QRectF GeometryItem::boundingRect() const {
    if (boundingRect_.isNull()) {
        boundingRect_ = shape().controlPointRect();
    }
    return boundingRect_;
}

QPainterPath GeometryItem::shape() const {
    const auto pen = GeometryItem::pen();
    if (path_ == QPainterPath{} || pen == Qt::NoPen)
        return path_;

    QPainterPathStroker ps;
    ps.setCapStyle(pen.capStyle());
    ps.setWidth(pen.width() ? pen.width() : cosmeticPenOutlineWidth);
    ps.setJoinStyle(pen.joinStyle());
    ps.setMiterLimit(pen.miterLimit());
    QPainterPath p = ps.createStroke(path_);
    p.addPath(path_);
    return p;
}

QPainterPath GeometryItem::opaqueArea() const {
    if (brush().isOpaque())
        return isClipped() ? clipPath() : shape();
    return BaseType::opaqueArea();
}

void GeometryItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*,
                         QWidget* widget) {
    Q_ASSERT(painter);

    auto pen = this->pen();

    bool cosmetic = true;

    if (widget && qobject_cast<QOpenGLWidget*>(widget)) {
        // Qt opengl bug when rendering with cosmetic pen
        // see #179
        auto graphicsView
            = qobject_cast<QGraphicsView*>(widget->parentWidget());
        if (graphicsView) {
            double scale = 1.f
                           / (std::sqrt(std::fabs(
                               graphicsView->transform().determinant())));
            cosmetic = scale >= 0.5;
        }
    }

    pen.setCosmetic(cosmetic);

    painter->setPen(pen);
    painter->setBrush(brush());

    painter->drawPath(path_);
}

QVariant GeometryItem::itemChange(GraphicsItemChange change,
                                  const QVariant& value) {
    switch (change) {
    case QGraphicsItem::ItemSceneChange: {
        if (qvariant_cast<QGraphicsScene*>(value)) {
            auto* const currentScene = scene();
            if (!currentScene) { // first insertion
                updatePath();
            }
            emit adding();
        } else {
            emit removing();
        }
        break;
    }
    case QGraphicsItem::ItemSceneHasChanged: {
        if (scene()) {
            emit added();
        } else {
            emit removed();
        }
        break;
    }
    case QGraphicsItem::ItemSelectedHasChanged: {
        emit selectedChanged(this->isSelected());
        break;
    }
    default:
        break;
    }
    return BaseType::itemChange(change, value);
}

GeometryItem* GeometryItem::fromGeometry(const OGRGeometry* geom,
                                         OGRSpatialReference* srcSrs,
                                         QGraphicsItem* parent) {
    if (!geom)
        return nullptr;

    struct CleverDeleter {
        explicit CleverDeleter(const OGRGeometry* original)
            : original_{original} {}

        void operator()(OGRGeometry* geom) {
            if (geom != original_)
                delete geom;
        }

    private:
        const OGRGeometry* const original_;
    };

    const auto* srs = geom->getSpatialReference();
    std::unique_ptr<OGRGeometry, CleverDeleter> spatializedGeom{
        nullptr, CleverDeleter{geom}};
    if (!srs) {
        if (srcSrs) {
            spatializedGeom.reset(const_cast<OGRGeometry*>(geom->clone()));
            spatializedGeom->assignSpatialReference(srcSrs);
            srs = srcSrs;
        } else
            return nullptr;
    } else {
        spatializedGeom.reset(const_cast<OGRGeometry*>(geom));
    }
    auto* const sceneSrs = Map::spatialReference();
    Q_ASSERT(spatializedGeom);
    if (!srs->IsSame(sceneSrs)) {
        if (spatializedGeom.get() == geom)
            spatializedGeom.reset(geom->clone());

        if (spatializedGeom->transformTo(sceneSrs) != OGRERR_NONE) {
            return nullptr;
        }
    }
    geom = spatializedGeom.get(); // shortcut for readability
    const auto type = wkbFlatten(geom->getGeometryType());
    GeometryItem* item{nullptr};
    switch (type) {
    case wkbPoint: {
        auto* const p = static_cast<const OGRPoint*>(geom);
        item = new PointItem{p, parent};
        break;
    }
    case wkbLineString: {
        auto* const ls = static_cast<const OGRLineString*>(geom);
        item = new LineStringItem{ls, parent};
        break;
    }
    case wkbPolygon: {
        auto* const pol = static_cast<const OGRPolygon*>(geom);
        item = new PolygonItem{pol, parent};
        break;
    }
    case wkbUnknown:
    default:
        break;
    }

    if (item) {
        item->updatePath(true);
    }

    return item;
}

double GeometryItem::get_Length() const { return mapwidget::get_Length(geom_); }

double GeometryItem::get_Area() const { return mapwidget::get_Area(geom_); }

PointItem::PointItem(const OGRPoint* point, QGraphicsItem* parent)
    : GeometryItem{point, parent}, editedChanging_{false} {
    setFlag(GraphicsItemFlag::ItemIsMovable, !geom_);
    setFlag(ItemIgnoresTransformations, true);
}

bool PointItem::isEdited() const {
    return flags() & QGraphicsItem::ItemIsMovable;
}

void PointItem::setEdited(bool edited) {
    setFlag(QGraphicsItem::ItemIsMovable, edited);
}

void PointItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*,
                      QWidget*) {
    Q_ASSERT(painter);

    // stroke won't depend on scale: item ignores transformations
    auto pen = this->pen();
    painter->setPen(pen);
    painter->setBrush(brush());
    painter->drawPath(path_);
}

void PointItem::updatePath(bool) {
    prepareGeometryChange();

    boundingRect_ = QRect{};
    path_ = QPainterPath{};
    path_.addPath(symbolPath());
    if (geom_) {
        QPointF p{static_cast<OGRPoint*>(geom_)->getX(),
                  static_cast<OGRPoint*>(geom_)->getY()};
        auto* const parent = parentItem();
        const auto pos = parent ? parent->mapFromScene(p) : p;
        setPos(pos);
    }
    emit pathUpdated();
    update();
}

void PointItem::updateItemFromGeometry() {
    if (geom_) {
        auto* const point = static_cast<OGRPoint*>(geom_);
        this->setPos(point->getX(), point->getY());
    }
}

void PointItem::updateGeometryFromItem() {
    const auto pos = QGraphicsItem::scenePos();
    if (!geom_)
        geom_ = new OGRPoint{pos.x(), pos.y()};
    else {
        static_cast<OGRPoint*>(geom_)->setX(pos.x());
        static_cast<OGRPoint*>(geom_)->setY(pos.y());
    }
    if (!geom_->getSpatialReference()) {
        auto* const sceneSrs = Map::spatialReference();
        geom_->assignSpatialReference(sceneSrs);
    }
    updatePath();
}

void PointItem::styleHasChanged() {

    // point shape and radius may change so symbolPath should be updated
    updatePath();
    GeometryItem::styleHasChanged();
}

void PointItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    Q_ASSERT(event);

    if (isEdited() && event->button() == Qt::LeftButton) {
        QApplication::setOverrideCursor(QCursor{Qt::ClosedHandCursor});
    }
    GeometryItem::mousePressEvent(event);
}

void PointItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    Q_ASSERT(event);
    if (event->button() == Qt::LeftButton) {
        if (QApplication::overrideCursor()) {
            QApplication::restoreOverrideCursor();
        }
    }
    GeometryItem::mouseReleaseEvent(event);
}

QVariant PointItem::itemChange(QGraphicsItem::GraphicsItemChange change,
                               const QVariant& value) {
    switch (change) {
    case QGraphicsItem::ItemPositionChange: {
        if (isEdited()) {
            emit moving(value.toPointF());
        }
        break;
    }
    case QGraphicsItem::ItemPositionHasChanged: {
        const auto edited = isEdited();
        if (edited) {
            updateGeometryFromItem(); // is it really needed to keep
                                      // geometry accurate?

            emit moved(value.toPointF());
        }
        break;
    }

    case QGraphicsItem::ItemFlagsChange: {
        const auto currentFlags = GeometryItem::flags();
        auto flags = static_cast<GraphicsItemFlags>(value.toInt());
        if ((currentFlags & GraphicsItemFlag::ItemIsMovable)
            != (flags & GraphicsItemFlag::ItemIsMovable)) { // changing
                                                            // ItemIsMovable

            if ((flags & GraphicsItemFlag::ItemIsMovable)
                && data(CustomQt::UserCantEditGeometry).toBool()) {
                Q_ASSERT(!isEdited());
                flags ^= GraphicsItemFlag::ItemIsMovable;
                return static_cast<int>(flags);
            }
            editedChanging_ = true;
        }
        break;
    }
    case QGraphicsItem::ItemFlagsHaveChanged: {
        if (editedChanging_) {
            auto flags = static_cast<GraphicsItemFlags>(value.toInt());
            editedChanged(flags & GraphicsItemFlag::ItemIsMovable);
            editedChanging_ = false;
        }
        break;
    }
    default:
        break;
    }
    return GeometryItem::itemChange(change, value);
}

Vertice::Vertice(const OGRPoint* geom, WithVerticesItem* parent)
    : PointItem{geom, parent} {
    Q_ASSERT(parent);
}

Vertice::~Vertice() {
    auto* const parentItem
        = static_cast<WithVerticesItem*>(PointItem::parentItem());
    parentItem->removingVertice(this);
}

QPen Vertice::pen() const {
    if (currentStyle_ == "current-vertice") {
        return PointItem::pen();
    }
    auto* const parentItem
        = static_cast<WithVerticesItem*>(PointItem::parentItem());
    auto vpen = resolveStyleComponent(parentItem, parentItem->currentStyle(),
                                      "point-pen");
    if (vpen.isValid()) {
        return qvariant_cast<QPen>(vpen);
    }

    return PointItem::pen();
}

QBrush Vertice::brush() const {
    if (currentStyle_ == "current-vertice") {
        return PointItem::brush();
    }
    auto* const parentItem
        = static_cast<WithVerticesItem*>(PointItem::parentItem());
    auto vpen = resolveStyleComponent(parentItem, parentItem->currentStyle(),
                                      "point-brush");
    if (vpen.isValid()) {
        return qvariant_cast<QBrush>(vpen);
    }

    return PointItem::brush();
}

QVariant Vertice::itemChange(GraphicsItemChange change, const QVariant& value) {
    switch (change) {
    case QGraphicsItem::ItemPositionHasChanged: {
        auto* const parentItem
            = static_cast<WithVerticesItem*>(PointItem::parentItem());
        parentItem->movingVertice(this);
        break;
    }
    case QGraphicsItem::ItemSceneHasChanged: {
        if (!qvariant_cast<QGraphicsScene*>(value)) { // scene removal
            auto* const parentItem
                = static_cast<WithVerticesItem*>(PointItem::parentItem());
            parentItem->removingVertice(this);
            return QVariant{}; // ignored
        }
        break;
    }
    case QGraphicsItem::ItemFlagsChange: {
        const auto currentFlags = GeometryItem::flags();
        auto flags = static_cast<GraphicsItemFlags>(value.toInt());
        if ((currentFlags & GraphicsItemFlag::ItemIsMovable)
            != (flags & GraphicsItemFlag::ItemIsMovable)) { // changing
                                                            // ItemIsMovable

            if (!(flags
                  & GraphicsItemFlag::ItemIsMovable)) { // When deselecting a
                                                        // vertice, propagate to
                                                        // parent, to deselect
                                                        // the whole polygon
                auto* const parentItem
                    = static_cast<WithVerticesItem*>(PointItem::parentItem());
                if (parentItem) {

                    parentItem->setEdited(false);
                    return QVariant{}; // ignored on vertice itself, as parent
                                       // will in turn update all points
                }
            }
        }
        break;
    }
    default:
        break;
    }
    return PointItem::itemChange(change, value);
}

void Vertice::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    // Resolve issue #111
    auto* const parent = parentItem();
    const auto pos
        = parent ? parent->mapFromScene(event->scenePos()) : event->scenePos();
    setPos(pos);
}

WithVerticesItem::WithVerticesItem(const OGRGeometry* geom,
                                   QGraphicsItem* parent)
    : GeometryItem{geom, parent}, currentIndex_{-1}, isEdited_{false} {
    setFlag(QGraphicsItem::ItemIsMovable, false);
}

WithVerticesItem::~WithVerticesItem() {
    auto it = vertices_.begin();
    while (it != vertices_.end()) {
        auto* const vert = *it;
        /* Do not invoke vert->setEdited(false) here,
         * as the vertice is about to be deleted, while
         * ItemFlagChange is triggered
         */
        it = vertices_.erase(it); // erase first so that
                                  // removingPoint won't update
                                  // geometry...

        delete vert;
    }
    Q_ASSERT(vertices_.empty());
}

bool WithVerticesItem::isEdited() const { return isEdited_; }

void WithVerticesItem::setEdited(bool edited) {
    if (isEdited_ == edited) {
        return;
    }
    isEdited_ = edited;
    editedChanged(edited);
}

Vertice* WithVerticesItem::vertice(int index) const {
    return (0 <= index && index < static_cast<int>(vertices_.size()))
               ? vertices_.at(index)
               : nullptr;
}

int WithVerticesItem::index(const Vertice* vert) const {
    if (!vert)
        return -1;

    const auto s = std::find(std::begin(vertices_), std::end(vertices_), vert);
    if (s != std::end(vertices_)) {
        const int index
            = static_cast<int>(std::distance(std::begin(vertices_), s));
        return index;
    }

    return -1;
}

int WithVerticesItem::verticeCount() const { return vertices_.size(); }

void WithVerticesItem::addVertice(const QPointF& sceneCoord, int index) {
    if (!isEdited())
        return;

    addVerticeSilently(sceneCoord, index);
    emit verticeAdded(index);

    updateGeometryFromItem();
    updatePath();
}

void WithVerticesItem::addVerticeSilently(const QPointF& sceneCoord,
                                          int index) {
    auto* const vert = new Vertice{nullptr, this};
    vert->setPos(mapFromScene(sceneCoord));

    const auto it = (index < 0 || index >= static_cast<int>(vertices_.size()))
                        ? std::end(vertices_)
                        : std::next(std::begin(vertices_), index);
    vertices_.insert(it, vert);

    connect(vert, &Vertice::removed, this, &WithVerticesItem::verticeRemoved);
}

void WithVerticesItem::setVertice(const QPointF& sceneCoord, int index) {
    if (!isEdited())
        return;

    if (index < 0 || index >= static_cast<int>(vertices_.size()))
        return;

    auto* const vert = vertices_.at(index);
    vert->setPos(mapFromScene(sceneCoord));
}

void WithVerticesItem::removeVertice(int index) {
    if (!isEdited())
        return;

    if (index < 0 || index >= static_cast<int>(vertices_.size()))
        return;

    if (data(CustomQt::UserCantRemove).toBool()
        && verticeCount() == minimalVerticesCount()) {
        return;
    }

    setCurrentIndex(-1);

    auto* const vert = vertices_.at(index);
    delete vert;
    // removing the vertice from list and updating the geometry
    // will be done in removingVertice
}

int WithVerticesItem::currentIndex() const { return currentIndex_; }

void WithVerticesItem::setCurrentIndex(int currentIndex) {

    auto oldCurrent = vertice(currentIndex_);
    if (oldCurrent) {
        oldCurrent->setCurrentStyle("edited");
    }
    auto current = vertice(currentIndex);
    if (current && current->isVisible()) {
        currentIndex_ = currentIndex;
        current->setCurrentStyle("current-vertice");
    } else {
        currentIndex_ = -1;
    }
    emit currentIndexChanged(currentIndex_);
}

void WithVerticesItem::editedChanged(bool edited) {
    GeometryItem::editedChanged(edited);

    if (!edited) {
        clearVertices();
    }
}

void WithVerticesItem::movingVertice(Vertice* vert) {
    if (!vert)
        return;

    const auto s = std::find(std::begin(vertices_), std::end(vertices_), vert);
    if (s != std::end(vertices_)) {
        // when geometry is converted to items, this condition prevent
        // erasing geometry
        const int index
            = static_cast<int>(std::distance(std::begin(vertices_), s));
        emit verticeMoved(index);

        updatePath();
        updateGeometryFromItem();
    }
}

void WithVerticesItem::removingVertice(Vertice* vert) {
    if (!vert)
        return;

    const auto s = std::find(std::begin(vertices_), std::end(vertices_), vert);
    if (s != std::end(vertices_)) {
        const int index
            = static_cast<int>(std::distance(std::begin(vertices_), s));
        emit aboutToRemoveVertice(index);

        vertices_.erase(s);

        // don't delete! it's called form vert ItemChange()

        updateGeometryFromItem();
        updatePath();

        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();
    }
}

void WithVerticesItem::clearVertices() {
    auto it = vertices_.begin();
    while (it != vertices_.end()) {
        auto* const vert = *it;
        it = vertices_.erase(it); // erase first so that following
                                  // statements won't update
                                  // geometry...
                                  /* Do not invoke vert->setEdited(false) here,
                                   * as the vertice is about to be deleted, while
                                   * ItemFlagChange is triggered
                                   */
        delete vert;
    }
    Q_ASSERT(vertices_.empty());
}

LineStringItem::LineStringItem(const OGRLineString* linestring,
                               QGraphicsItem* parent)
    : WithVerticesItem{linestring, parent} {
    if (!geom_) {
        setEdited(true);
    }
}

QBrush LineStringItem::brush() const {
    auto brush = WithVerticesItem::brush();
    brush.setStyle(Qt::NoBrush); // won't fill shapes
    return brush;
}

void LineStringItem::updatePath(bool geometryChange) {
    if (isEdited()) {
        prepareGeometryChange();

        boundingRect_ = QRect{};
        path_ = QPainterPath{};
        if (static_cast<int>(vertices_.size()) > 1) {
            path_.moveTo(vertices_.at(0)->pos());
            for (int i = 1; i < verticeCount(); ++i)
                path_.lineTo(vertices_.at(i)->pos());
        }
    } else if (geom_ && geometryChange) {
        prepareGeometryChange();

        boundingRect_ = QRect{};
        path_ = QPainterPath{};

        auto* const ls = static_cast<OGRLineString*>(geom_);
        if (ls->getNumPoints() > 1) {
            path_.moveTo(QPointF{ls->getX(0), ls->getY(0)});

            for (int i = 1; i < ls->getNumPoints(); ++i) {
                path_.lineTo(QPointF{ls->getX(i), ls->getY(i)});
            }
        }
    }

    if (label_) {
        updateLabelPosition();
    }
    emit pathUpdated();
    update();
}

void LineStringItem::updateItemFromGeometry() {
    clearVertices();
    if (geom_) {
        auto* const ls = static_cast<OGRLineString*>(geom_);
        for (int i = 0; i < ls->getNumPoints(); ++i) {
            const QPointF pos{ls->getX(i), ls->getY(i)};
            addVerticeSilently(pos, i);
        }
    }
}

void LineStringItem::updateGeometryFromItem() {
    if (!geom_)
        geom_ = new OGRLineString;

    const auto numPoints = static_cast<int>(vertices_.size());
    static_cast<OGRLineString*>(geom_)->setNumPoints(numPoints);
    for (int i = 0; i < numPoints; ++i) {
        auto* const vert = vertices_.at(i);
        const auto pointPos = mapToScene(vert->pos());
        static_cast<OGRLineString*>(geom_)->setPoint(i, pointPos.x(),
                                                     pointPos.y());
    }
    if (!geom_->getSpatialReference()) {
        auto* const sceneSrs = Map::spatialReference();
        geom_->assignSpatialReference(sceneSrs);
    }
}

void LineStringItem::updateLabelPosition() {
    // update label position
    if (!geom_ || !label_)
        return;

    auto lineString = static_cast<OGRLineString*>(geom_);
    if (lineString->getNumPoints() < 2) {
        label_->setVisible(false);
        return;
    }
    // middle of the first segment
    label_->setVisible(true);
    OGRPoint point0;
    lineString->getPoint(0, &point0);
    OGRPoint point1;
    lineString->getPoint(1, &point1);

    QPointF posLabel{(point0.getX() + point1.getX()) * 0.5,
                     (point0.getY() + point1.getY()) * 0.5};

    label_->setPos(mapFromScene(posLabel));
    label_->update();
}

QPainterPath LineStringItem::shape() const {
    const auto pen = GeometryItem::pen();
    if (path_ == QPainterPath{} || pen == Qt::NoPen)
        return path_;

    // For lines, we cannot directly use the path for the shape, as inherited
    // from GeometryItem: it is interpreted as a closed path for hit tests, and
    // therefore clicking inside the resulting polygon would select the item,
    // which is pretty confusing.
    // Therefore, we concatenate the reversed path, so that it follows the line
    // string from beginning to end, and then back to beginning, with some
    // tolerance around the lines corresponding to the pen stroke width.
    QPainterPathStroker ps;
    ps.setCapStyle(pen.capStyle());
    ps.setWidth(pen.width() ? pen.width() : cosmeticPenOutlineWidth);
    ps.setJoinStyle(pen.joinStyle());
    ps.setMiterLimit(pen.miterLimit());
    QPainterPath closed;
    closed.connectPath(path_);
    closed.connectPath(path_.toReversed());

    QPainterPath p = ps.createStroke(closed);
    p.addPath(closed);

    return p;
}

PolygonItem::PolygonItem(const OGRPolygon* polygon, QGraphicsItem* parent)
    : WithVerticesItem{polygon, parent} {
    if (!geom_) {
        setEdited(true);
    }
}

void PolygonItem::updatePath(bool geometryChange) {
    if (isEdited()) {
        // update from item vertices
        prepareGeometryChange();

        boundingRect_ = QRect{};
        path_ = QPainterPath{};
        if (static_cast<int>(vertices_.size()) > 1) {
            QPolygonF polygon;
            for (int i = 0; i < verticeCount(); ++i)
                polygon << vertices_.at(i)->pos();
            polygon << polygon.first();
            path_.addPolygon(polygon);
        }
    } else if (geom_ && geometryChange) {
        // update from item geometry

        prepareGeometryChange();

        boundingRect_ = QRect{};
        path_ = QPainterPath{};

        auto* const pol = static_cast<OGRPolygon*>(geom_);
        auto* const lr = pol->getExteriorRing();
        if (lr) {
            QPolygonF polygon;
            for (int i = 0; i < lr->getNumPoints(); ++i) {
                polygon << QPointF(lr->getX(i), lr->getY(i));
            }
            path_.addPolygon(polygon);
        }
    }

    if (label_) {
        updateLabelPosition();
    }
    emit pathUpdated();
    update();
}

void PolygonItem::updateItemFromGeometry() {
    clearVertices();

    if (geom_) {
        auto* const pol = static_cast<OGRPolygon*>(geom_);
        auto* const lr = pol->getExteriorRing();
        if (lr) {
            for (int i = 0;
                 i < lr->getNumPoints() - 1; // a linear ring is closed
                 ++i) {
                const QPointF pos{lr->getX(i), lr->getY(i)};
                addVerticeSilently(pos, i);
            }
        }
    }
}

void PolygonItem::updateLabelPosition() {
    if (!label_) {
        return;
    }

    if (geom_) {
        auto lr = static_cast<OGRPolygon*>(geom_)->getExteriorRing();
        if (lr->getNumPoints() >= 4) { // ring implies closed
            OGRPoint pos;
            label_->setVisible(true);
            auto* const hull = geom_->ConvexHull();
            if (hull) {
                if (hull->Centroid(&pos) == OGRERR_NONE) {
                    label_->setPos(mapFromScene(pos.getX(), pos.getY()));
                } else {
                    label_->setVisible(false);
                }
                delete hull;
            }
        } else {
            label_->setVisible(false);
        }
    } else {
        label_->setVisible(false);
    }
    label_->update();
}

void PolygonItem::updateGeometryFromItem() {
    if (geom_) {
        delete geom_; // no way to change exterior ring
        geom_ = nullptr;
    }
    OGRLinearRing lr;
    lr.setNumPoints(vertices_.size());

    auto* const sceneSrs = Map::spatialReference();
    lr.assignSpatialReference(sceneSrs);

    for (int i = 0; i < static_cast<int>(vertices_.size()); ++i) {
        auto* const vert = vertices_.at(i);
        const auto pointPos = mapToScene(vert->pos());
        lr.setPoint(i, pointPos.x(), pointPos.y());
    }
    lr.closeRings();

    if (lr.getNumPoints() < 4) {
        return;
    }

    geom_ = new OGRPolygon;
    static_cast<OGRPolygon*>(geom_)->addRing(&lr);
    geom_->assignSpatialReference(sceneSrs);
}
