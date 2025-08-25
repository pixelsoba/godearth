#pragma once

#include <QImage>
#include <QObject>
#include <QVariant>

#include "mapwidget/tilesources/localtilesource.h"

class ElevationHelper;
class TileItemGroup;
class TileItem;
class GDALDataset;

// Responsible of generation of elevation tiles
//
// Instances are expected to be moved to a dedicated worker thread.
class MAPWIDGET_LIBRARY ElevationTileWorker : public TileWorker {
    Q_OBJECT

public:
    ElevationTileWorker(int tileSize, ElevationHelper* helper);

    void operator()(TilePromise* promise) override;

    void setFolderCacheName(const QString& folderCacheName);

    void setTilePathTemplate(const QString& tilePathTemplate);

private:
    int tileSize_;

    ElevationHelper* const helper_;

    QTemporaryFile reliefColorsFile_;

    QString folderCacheName_;

    QString tilePathTemplate_;

    void initColorsFile();

    QImage getColorReliefImg(GDALDataset* src) const;

    QImage getHillShadeImg(GDALDataset* src) const;
};

// Implement a tile source where tiles are elevation tiles
class MAPWIDGET_LIBRARY ElevationTileSource
    : public LocalTileSourceAbstractXYZ {
    Q_OBJECT

public:
    ElevationTileSource(ElevationHelper* helper, const QString& name,
                        const QString& folderCacheName = "",
                        const QString& tilePathTemplate = "",
                        int minZoomLevel = 9, int maxZoomLevel = 22,
                        int tileSize = 256, const QString& legalString = "");

    std::shared_ptr<ElevationSource> elevationSource() const;

    void setFolderCacheName(const QString& folder) override;

    void setTilePathTemplate(const QString& tilePathTemplate) override;

    QGraphicsItemGroup* getExtentItem() override;

    TilePromise* createTile(const QVariant& tileKey,
                            TileItemGroup* group) const override;

    void clearCache();

    ElevationHelper* helper() const;

private:
    ElevationHelper* const helper_;

};
