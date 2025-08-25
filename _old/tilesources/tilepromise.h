#pragma once

#include <QObject>
#include <QVariant>

#include "mapwidget/global.h"

class TileItem;
class TileItemGroup;

class MAPWIDGET_LIBRARY TilePromise : public QObject {

    Q_OBJECT

public:
    explicit TilePromise(const QVariant& tileKey, TileItemGroup* group,
                         QObject* parent = nullptr)
        : QObject{parent}, tileKey_{tileKey}, group_{group} {}

    QVariant tileKey() const { return tileKey_; }

    virtual bool isFinished() const = 0;

    virtual void abort() = 0;

    virtual TileItem* item() const = 0;

signals:
    void finished();

protected:
    QVariant tileKey_;

    TileItemGroup* const group_;
};
