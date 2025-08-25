#include "mapwidget/graphicsitems/tileitem.h"

#include <QGraphicsProxyWidget>
#include <QPixmapCache>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

#include "mapwidget/graphicsitems/tileitemgroup.h"
#include "mapwidget/graphicsitems/tileitemgrouphelper.h"
#include "mapwidget/map.h"
#include "mapwidget/tilesources/tilesource.h"
#include "mapwidget/tilesources/xyz.h"
#include "mapwidget/views/mapview.h"

TileItem::TileItem(TileItemGroup* parent, const QVariant& tileKey)
    : tileKey_{tileKey} {
    Q_ASSERT(tileKey_.value<XYZ>().valid()); // for now, support for
                                             // XYZ only

    setParentItem(parent);

    setFlag(ItemIsSelectable, false);
    setData(CustomQt::UserCantEditGeometry, true);
    setData(CustomQt::UserCantRemove, true);

    // when painting on an OpenGL surface, setting cache mode to
    // QGraphicsItem::DeviceCoordinateCache or
    // QGraphicsItem::ItemCoordinateCache leaves the surface blank...

    setTransformationMode(Qt::SmoothTransformation);
    setTransform(QTransform::fromScale(1, -1), false);

    parent->addToGroup(this);
}

TileItem::~TileItem() {}

int TileItem::level() const {
    const auto xyz = tileKey_.value<XYZ>();
    return xyz.z();
}

void TileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                     QWidget* widget) {
    if (!painter || !option || !widget)
        return;

    const bool hasAntialising = painter->renderHints() & QPainter::Antialiasing;
    if (hasAntialising)
        painter->setRenderHint(QPainter::Antialiasing, false);
    // hack: enabling antialiasing for tiles seems to introduce white
    // lines between the tiles

    const auto* const view = static_cast<MapView*>(widget->parent());
    const auto res = view->resolution();
    const auto* const itemGroup = static_cast<TileItemGroup*>(parentItem());
    const auto* const itemGroupHelper = itemGroup->helper();
    if (itemGroupHelper->mustDrawTileItem(this, res)) {
        const QPixmap p = pixmap();
        if (boundingRect_.isNull())
            prepareGeometryChange();
        painter->drawPixmap(boundingRect(), p, p.rect());
    }

    if (hasAntialising)
        painter->setRenderHint(QPainter::Antialiasing, true);
}

QRectF TileItem::boundingRect() const {
    if (boundingRect_.isNull()) {
        auto source = TileItem::source();
        Q_ASSERT(source);

        const auto itemResolution = source->resolution(level());
        const int tileSize = source->tileSize();
        const auto size = itemResolution * tileSize;
        boundingRect_ = QRectF(0, 0, size, size);
    }
    return boundingRect_;
}

QVariant TileItem::itemChange(GraphicsItemChange change,
                              const QVariant& value) {
    switch (change) {
    case QGraphicsItem::ItemSceneHasChanged: {
        if (scene()) {
            const auto xyz = tileKey_.value<XYZ>();
            setPos(xyz.scenePos()); // parent is expected to be a
                                    // group item...
        }
        break;
    }
    default:
        break;
    }
    return QGraphicsPixmapItem::itemChange(change, value);
}

std::shared_ptr<TileSource> TileItem::source() const {
    const auto* const group = static_cast<TileItemGroup*>(parentItem());
    Q_ASSERT(group);

    const auto* const helper = group->helper();
    Q_ASSERT(helper);

    return helper->source();
}
