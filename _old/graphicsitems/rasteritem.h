#pragma once

#include <array>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>

#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QThread>

#include "mapwidget/global.h"
#include "mapwidget/map.h"

class GDALDataset;
class MapView;

typedef std::tuple<QRect, QSize, QRectF> PixmapKey;
// Components are expected to be the source image rectangle, the size
// in pixels of the destination rectangle, and the corresponding rect
// in item's coordinates

class MAPWIDGET_LIBRARY RasterDataPromise : public QObject {
public:
    enum State {
        Waiting = 0,
        Running = 1,
        Finished = 2,
    };

    enum StateDetail {
        Empty = 0, // means success if finished
        Failure = 1,
        Cancelled = 2
    };

    RasterDataPromise(const PixmapKey& key);

    ~RasterDataPromise();

    State state() const;

    StateDetail stateDetail() const;

    QPixmap* pixmap() { return pixmap_; }

    PixmapKey key() const { return key_; }

private:
    QPixmap* pixmap_;

    PixmapKey key_;

    State state_;

    StateDetail detail_;

    void setState(State state, StateDetail detail = Empty);

    friend class RasterDataWorker;
};

class MAPWIDGET_LIBRARY RasterDataWorker : public QObject {
    Q_OBJECT

public:
    explicit RasterDataWorker(std::shared_ptr<GDALDataset> dataset);

    void operator()(RasterDataPromise* promise);

    void notifyProgress(double percent);

    void abort();

    bool aborted() const;

signals:
    void finished(RasterDataPromise* promise);

    void progress(double percent);

    void wasAborted();

private:
    std::shared_ptr<GDALDataset> dataset_;

    int bandCount_;

    bool abort_;

    void resetAbortState();
};

class MAPWIDGET_LIBRARY RasterItem : public QGraphicsObject {
    Q_OBJECT

public:
    RasterItem(std::shared_ptr<GDALDataset> dataset);

    ~RasterItem() override;

signals:
    void operate(RasterDataPromise* promise);

    void workerProgress(double percent);

protected:
    QVariant itemChange(GraphicsItemChange change,
                        const QVariant& value) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    QRectF boundingRect() const override { return boundingRect_; }

private:
    QRectF boundingRect_; // item coordinates

    QSize rasterSize_; // pixels

    Map* map_; // listened map

    Map::ViewListenerType listener_;

    typedef std::unordered_set<RasterDataPromise*> PixmapPromiseIndex;
    PixmapPromiseIndex promises_;
    // will contain at most one pixmap per view (usualy there's only
    // one view...)

    std::map<MapView*, RasterDataPromise*> schedule_;

    std::shared_ptr<GDALDataset> dataset_;

    std::array<double, 6> trans_;

    QThread thread_;

    QTimer* const timer_;

    RasterDataWorker* worker_;

    void scheduleRasterRead(
        const std::map<MapView*, std::pair<QRectF, qreal>>& rects);

    void deleteObsoletePromises(const PixmapPromiseIndex& keep);

    void launchScheduledPromises();

    void handleResult(RasterDataPromise* promise);

    QPoint scene2image(const QPointF& p) const;

    PixmapKey sceneRectKey(QRectF sceneRect, double res) const;
};
