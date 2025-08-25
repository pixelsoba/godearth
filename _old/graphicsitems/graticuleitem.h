#pragma once

#include <QBrush>
#include <QFont>
#include <QGraphicsItem>
#include <QPen>

class GraticuleItem : public QGraphicsItem {
public:
    explicit GraticuleItem(QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    QPen unitPen() const { return unitPen_; }

    void setUnitPen(const QPen& pen);

    QPen decimalPen() const { return decimalPen_; }

    void setDecimalPen(const QPen& pen);

    QPen centesimalPen() const { return centesimalPen_; }

    void setCentesimalPen(const QPen& pen);

    QBrush unitBrush() const { return unitBrush_; }

    void setUnitBrush(const QBrush& brush);

    QBrush decimalBrush() const { return decimalBrush_; }

    void setDecimalBrush(const QBrush& brush);

    QBrush centesimalBrush() const { return centesimalBrush_; }

    void setCentesimalBrush(const QBrush& brush);

private:
    QPen unitPen_;

    QPen decimalPen_;

    QPen centesimalPen_;

    QBrush unitBrush_;

    QBrush decimalBrush_;

    QBrush centesimalBrush_;

    QFont font_;
};
