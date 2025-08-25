#pragma once

#include <memory>

#include <QGraphicsPixmapItem>
#include <QVariant>

#include "mapwidget/global.h"
#include "mapwidget/graphicsitems/common.h"
#include "mapwidget/graphicsitems/tileitemgroup.h"

class TileSource;

class MAPWIDGET_LIBRARY TileItem : public QGraphicsPixmapItem {
public:
    TileItem(TileItemGroup* parent, const QVariant& tileKey);

    ~TileItem() override;

    int level() const;

    QVariant tileKey() const { return tileKey_; }

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    QRectF boundingRect() const override;

    QVariant itemChange(GraphicsItemChange change,
                        const QVariant& value) override;

private:
    const QVariant tileKey_;

    mutable QRectF boundingRect_;

    std::shared_ptr<TileSource> source() const;
};
