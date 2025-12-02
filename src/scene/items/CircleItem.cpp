#include "CircleItem.h"

#include <QBrush>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPen>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtMath>
#include "model/MaterialModel.h"

namespace {
constexpr double kMinRadiusPx = 1.0;
constexpr double kMaxRadiusPx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
}

CircleItem::CircleItem(qreal radius, QGraphicsItem *parent)
    : QGraphicsEllipseItem(QRectF(-radius, -radius, 2*radius, 2*radius), parent) {
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setPen(QPen(Qt::black, 1.0));
    setBrush(QBrush(QColor(128, 128, 128, 128)));
    setTransformOriginPoint(boundingRect().center());
}

void CircleItem::set_name(const QString &name) {
    QString trimmed = name.trimmed();
    if (!trimmed.isEmpty()) {
        name_ = trimmed;
    }
}

QWidget* CircleItem::create_properties_widget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *form = new QFormLayout(widget);
    form->setContentsMargins(0, 0, 0, 0);

    auto *radiusSpin = new QDoubleSpinBox(widget);
    radiusSpin->setRange(kMinRadiusPx, kMaxRadiusPx);
    radiusSpin->setDecimals(1);
    radiusSpin->setValue(rect().width() / 2.0);
    QObject::connect(radiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget, [this, radiusSpin]{
        qreal r = radiusSpin->value();
        setRect(QRectF(-r, -r, 2*r, 2*r));
        setTransformOriginPoint(boundingRect().center());
        notify_geometry_changed();
    });

    auto *rotationSpin = new QDoubleSpinBox(widget);
    rotationSpin->setRange(kMinRotationDeg, kMaxRotationDeg);
    rotationSpin->setDecimals(1);
    rotationSpin->setSingleStep(5.0);
    rotationSpin->setSuffix("°");
    rotationSpin->setValue(rotation());
    QObject::connect(rotationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget, [this, rotationSpin]{
        setRotation(rotationSpin->value());
        notify_geometry_changed();
    });

    form->addRow("Radius:", radiusSpin);
    form->addRow("Rotation:", rotationSpin);

    return widget;
}

QJsonObject CircleItem::to_json() const {
    QJsonObject obj;
    obj["type"] = type_name();
    obj["name"] = name_;
    obj["position"] = QJsonArray{pos().x(), pos().y()};
    obj["rotation"] = rotation();
    obj["radius"] = rect().width() / 2.0;
    QColor c = brush().color();
    obj["fill_color"] = QJsonArray{c.red(), c.green(), c.blue(), c.alpha()};
    return obj;
}

void CircleItem::from_json(const QJsonObject &json) {
    if (json.contains("name")) {
        name_ = json["name"].toString();
    }
    if (json.contains("position")) {
        QJsonArray p = json["position"].toArray();
        setPos(p[0].toDouble(), p[1].toDouble());
    }
    if (json.contains("rotation")) {
        setRotation(json["rotation"].toDouble());
    }
    if (json.contains("radius")) {
        qreal r = json["radius"].toDouble();
        setRect(QRectF(-r, -r, 2*r, 2*r));
        setTransformOriginPoint(boundingRect().center());
    }
    if (json.contains("fill_color")) {
        QJsonArray c = json["fill_color"].toArray();
        setBrush(QBrush(QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt())));
    }
}

void CircleItem::set_geometry_changed_callback(std::function<void()> callback) {
    geometry_changed_callback_ = std::move(callback);
}

void CircleItem::clear_geometry_changed_callback() {
    geometry_changed_callback_ = nullptr;
}

void CircleItem::notify_geometry_changed() const {
    if (geometry_changed_callback_) {
        geometry_changed_callback_();
    }
}

void CircleItem::set_material_model(MaterialModel *material) {
    material_model_ = material;
    update(); // Trigger repaint to show/hide grid
}

void CircleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // Draw the base ellipse first
    QGraphicsEllipseItem::paint(painter, option, widget);

    // Draw grid if material has grid enabled
    if (material_model_ && material_model_->grid_type() == MaterialModel::GridType::Radial) {
        draw_radial_grid(painter, rect());
    }
}

void CircleItem::draw_radial_grid(QPainter *painter, const QRectF &rect) const {
    if (!material_model_) {
        return;
    }

    painter->save();

    QPen gridPen(QColor(0, 0, 0, 100)); // Semi-transparent black
    gridPen.setWidthF(0.5);
    painter->setPen(gridPen);

    const QPointF center = rect.center();
    const qreal radius = rect.width() / 2.0;
    const double frequency = material_model_->grid_frequency();

    // Frequency is "lines per 100px", so calculate spacing based on radius
    // For radial lines: number of lines = frequency * (radius / 100) * some multiplier
    // For circles: spacing = 100 / frequency
    const qreal spacing = 100.0 / frequency; // Distance between concentric circles in pixels

    // Draw radial lines (from center to edge) - more lines for larger circles
    const int numRadialLines = qMax(8, static_cast<int>(frequency * (radius / 100.0) * 2));
    for (int i = 0; i < numRadialLines; ++i) {
        const qreal angle = (360.0 * i) / numRadialLines;
        const qreal radians = qDegreesToRadians(angle);
        const QPointF endPoint = center + QPointF(radius * qCos(radians), radius * qSin(radians));
        painter->drawLine(center, endPoint);
    }

    // Draw concentric circles with spacing based on frequency
    qreal currentRadius = spacing;
    while (currentRadius < radius) {
        painter->drawEllipse(center, currentRadius, currentRadius);
        currentRadius += spacing;
    }

    painter->restore();
}

QVariant CircleItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
        notify_geometry_changed();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

