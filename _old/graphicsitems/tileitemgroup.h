#pragma once

#include <memory>

#include <QGraphicsItemGroup>

#include "mapwidget/global.h"
#include "mapwidget/graphicsitems/common.h"
#include "mapwidget/map.h"
#include "mapwidget/tilesources/tilesource.h"

class TileItemGroupHelper;

class MAPWIDGET_LIBRARY TileItemGroup : public QGraphicsItemGroup {
public:
    TileItemGroup(std::shared_ptr<TileSource> source);

    ~TileItemGroup() override;

    std::shared_ptr<TileSource> source() const { return source_; }

    TileItemGroupHelper* helper() const { return helper_; }

    QVariant itemChange(GraphicsItemChange change,
                        const QVariant& value) override;

    Map* map() const { return map_; }

    QGraphicsItemGroup* extentItem() const { return extentItem_; }

    void forceUpdate(const std::map<MapView*, std::pair<QRectF, qreal>>& rects);

    QPainterPath shape() const override;

    void setClipRect(const QRectF& clipRect);

private:
    std::shared_ptr<TileSource> source_;

    TileItemGroupHelper* helper_;

    Map* map_; // listened map

    Map::ViewListenerType listener_;

    QGraphicsItemGroup* extentItem_;

    QRectF clipRect_;
};
