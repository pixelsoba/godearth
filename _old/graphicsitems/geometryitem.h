#pragma once

#include <memory>

#include <QBrush>
#include <QFont>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVariant>

#include "mapwidget/global.h"

class OGRGeometry;
class OGRSpatialReference;
class OGRPoint;
class OGRLineString;
class OGRPolygon;
class OGRMultiPoint;
class OGRMultiLineString;
class OGRMultiPolygon;
class OGRGeometryCollection;
class QGraphicsSimpleTextItem;

// Base class for geometry items
//
// It is a composite class holding an OGR geometry, a QPainterPath
// and painting style data.
//
// A style is made of a QPen, a QBrush and a QFont. Style definitions
// are stored as a QVariant map and can be fetched and set using the
// styleMap() and setStyleMap() functions. The current style can be
// accessed and set with currentStyle() and setCurrentStyle(). The
// functions pen(), brush() and font() recursively check the item
// ancestors for the current style definition.
class MAPWIDGET_LIBRARY GeometryItem : public QGraphicsObject {
    Q_OBJECT

    typedef QGraphicsObject BaseType; // class wasn't written with
                                      // QObject inheritance in mind

public:
    ~GeometryItem() override;

    // Geometry

    OGRGeometry* geometry(OGRSpatialReference* srs = nullptr) const;
    // returned geometry is a clone, the caller MUST free allocated
    // resources

    // Edition

    virtual bool isEdited() const { return false; }

    virtual void setEdited(bool edited) { Q_UNUSED(edited); }

    void setGeometry(const OGRGeometry* geom,
                     OGRSpatialReference* srs = nullptr);

    // Style

    typedef QVariantMap Style;    // keys: pen, brush, font
    typedef QVariantMap StyleMap; // keys are style names

    QString currentStyle() const;

    void setCurrentStyle(const QString& id);

    StyleMap styleMap() const;

    void setStyleMap(const StyleMap& styleMap);

    virtual QPen pen() const;

    virtual QBrush brush() const;

    virtual QFont font() const;

    virtual QVariant resolveCurrentStyleComponent(const QString& key) const;

    // Label

    QGraphicsSimpleTextItem* label() const;

    QString labelText() const;

    void setLabelText(const QString& text);

    void clearLabel();

    bool labelIsVisible() const;

    void setLabelVisible(bool visible);

    // QGraphicsItem reimplementations

    QRectF boundingRect() const override;

    QPainterPath shape() const override;

    QPainterPath opaqueArea() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    // Factory

    static GeometryItem* fromGeometry(const OGRGeometry* geom,
                                      OGRSpatialReference* srcSrs,
                                      QGraphicsItem* parent = nullptr);

    double get_Length() const;

    double get_Area() const;

signals:
    void adding();

    void added();

    void moving(const QPointF& sceneCoord);

    void moved(const QPointF& sceneCoord);

    void removing();

    void removed();

    void editingChanged(bool edited);

    void selectedChanged(bool selected);

    void pathUpdated();

protected:
    OGRGeometry* geom_;

    QPainterPath path_;

    QString currentStyle_;

    mutable QRectF boundingRect_; // cached bounding rect

    QGraphicsSimpleTextItem* label_;

    explicit GeometryItem(const OGRGeometry* geom,
                          QGraphicsItem* parent = nullptr);

    virtual void styleHasChanged();

    void setLabelStyle();

    QPainterPath symbolPath() const;

    virtual void updatePath(bool geometryChange = false);

    virtual void updateItemFromGeometry() = 0;

    virtual void updateGeometryFromItem() = 0;

    virtual void updateLabelPosition() {}

    virtual void editedChanged(bool edited);

    QVariant itemChange(GraphicsItemChange change,
                        const QVariant& value) override;
};

// Items based on simple geometries

class MAPWIDGET_LIBRARY PointItem : public GeometryItem {
    Q_OBJECT

public:
    explicit PointItem(const OGRPoint* point = nullptr,
                       QGraphicsItem* parent = nullptr);
    bool isEdited() const override;

    void setEdited(bool edited) override;

protected:
    bool editedChanging_;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    void updatePath(bool geometryChange = false) override;

    void updateItemFromGeometry() override;

    void updateGeometryFromItem() override;

    friend class WithVerticesItem; // to call updatePath()

    void styleHasChanged() override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    QVariant itemChange(GraphicsItemChange change,
                        const QVariant& value) override;
};

class WithVerticesItem;

class MAPWIDGET_LIBRARY Vertice : public PointItem {
    Q_OBJECT

public:
    Vertice(const OGRPoint* geom, WithVerticesItem* parent);

    ~Vertice() override;

    QPen pen() const override;

    QBrush brush() const override;

protected:
    QVariant itemChange(GraphicsItemChange change,
                        const QVariant& value) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
};

class MAPWIDGET_LIBRARY WithVerticesItem : public GeometryItem {
    Q_OBJECT

public:
    explicit WithVerticesItem(const OGRGeometry* geom = nullptr,
                              QGraphicsItem* parent = nullptr);

    ~WithVerticesItem() override;

    bool isEdited() const override;

    void setEdited(bool edited) override;

    Vertice* vertice(int index) const;

    int index(const Vertice* vert) const;

    int verticeCount() const; // WARNING! zero when non edited

    void addVertice(const QPointF& sceneCoord, int index = -1);

    void setVertice(const QPointF& sceneCoord, int index);

    void removeVertice(int index);

    virtual int minimalVerticesCount() = 0;

    int currentIndex() const;

    void setCurrentIndex(int currentIndex);

signals:
    void verticeAdded(int index);

    void verticeMoved(int index);

    void aboutToRemoveVertice(int index);

    void verticeRemoved();

    void currentIndexChanged(int index);

protected:
    std::vector<Vertice*> vertices_;

    int currentIndex_;

    bool isEdited_;

    void editedChanged(bool edited) override;

    void addVerticeSilently(const QPointF& sceneCoord, int index);

    void movingVertice(Vertice* vert);

    void removingVertice(Vertice* vert);

    void clearVertices();

    friend class Vertice; // access to verticeMoved() and removingVertice()
};

class MAPWIDGET_LIBRARY LineStringItem : public WithVerticesItem {
    Q_OBJECT

public:
    explicit LineStringItem(const OGRLineString* linestring = nullptr,
                            QGraphicsItem* parent = nullptr);

    QBrush brush() const override;

    QPainterPath shape() const override;

    int minimalVerticesCount() override { return 2; }

protected:
    void updatePath(bool geometryChange = false) override;

    void updateItemFromGeometry() override;

    void updateGeometryFromItem() override;

    void updateLabelPosition() override;
};

class MAPWIDGET_LIBRARY PolygonItem : public WithVerticesItem {
    Q_OBJECT

public:
    explicit PolygonItem(const OGRPolygon* polygon = nullptr,
                         QGraphicsItem* parent = nullptr);

    int minimalVerticesCount() override { return 3; }

protected:
    void updatePath(bool geometryChange = false) override;

    void updateItemFromGeometry() override;

    void updateGeometryFromItem() override;

    void updateLabelPosition() override;
};
