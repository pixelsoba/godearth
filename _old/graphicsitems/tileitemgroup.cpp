#include "mapwidget/graphicsitems/tileitemgroup.h"

#include <QPainter>
#include <functional>

#include "mapwidget/graphicsitems/common.h"
#include "mapwidget/graphicsitems/tileitemgrouphelper.h"
#include "mapwidget/map.h"
#include "mapwidget/models/foldermodel.h"

TileItemGroup::TileItemGroup(std::shared_ptr<TileSource> source)
    : source_{source},
      helper_{nullptr},
      map_{nullptr},
      listener_{nullptr},
      extentItem_{nullptr} {
    setData(CustomQt::FolderModelBehaviorKey, FolderModel::ExcludeChildItems);

    Q_ASSERT(source);
    setData(CustomQt::ItemNameKey, source->name());
    setData(CustomQt::UserCantEditGeometry, true);
    setData(CustomQt::LegalString, source ? source->legalString() : "");

    extentItem_ = source_->getExtentItem();

    helper_ = new TileItemGroupHelper{source, this};

    using namespace std::placeholders;
    listener_ = std::bind(&TileItemGroupHelper::fillWithTiles, helper_, _1);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape);
}

TileItemGroup::~TileItemGroup() {
    if (map_)
        map_->unregisterViewListener(&listener_);

    delete helper_;

    delete extentItem_;
}

QVariant TileItemGroup::itemChange(GraphicsItemChange change,
                                   const QVariant& value) {
    switch (change) {
    case QGraphicsItem::ItemSceneChange: {
        // at this stage, scene is not set
        auto* const scene = qvariant_cast<QGraphicsScene*>(value);
        auto* const sceneMap
            = scene ? qobject_cast<Map*>(scene->parent()) : nullptr;
        if (sceneMap != map_) {
            if (map_)
                map_->unregisterViewListener(&listener_);
            map_ = sceneMap;
            if (map_ && isVisible())
                map_->registerViewListener(&listener_);
        }
        break;
    }
    case QGraphicsItem::ItemVisibleChange: {
        if (map_) {
            const bool visible = value.toBool();
            if (visible) {
                map_->registerViewListener(&listener_);
                map_->triggerViewChanged();
            } else {
                helper_->abortPromises();
                map_->unregisterViewListener(&listener_);
            }
        }
        break;
    }
    default:
        break;
    }
    return QGraphicsItemGroup::itemChange(change, value);
}

void TileItemGroup::forceUpdate(
    const std::map<MapView*, std::pair<QRectF, qreal>>& rects) {
    source_->updateExtentItem();
    helper_->fillWithTiles(rects);
}

QPainterPath TileItemGroup::shape() const {
    QPainterPath path;
    if (!clipRect_.isNull()) {
        path.addRect(clipRect_);
        return path;
    } else if (extentItem_) {
        for (auto child : extentItem_->childItems()) {
            path.addRect(child->sceneBoundingRect());
        }
        return path;
    } else {
        path = QGraphicsItemGroup::shape();
    }
    return path;
}

void TileItemGroup::setClipRect(const QRectF& clipRect) {
    clipRect_ = clipRect;
}
