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
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <QtMath>
#include "model/MaterialModel.h"

namespace {
constexpr double kMinRadiusPx = 1.0;
constexpr double kMaxRadiusPx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
// Grid ring parameters (as fraction of radius)
constexpr double kInnerRingStartRatio = 0.5;  // Inner ring starts at 50% of radius
constexpr double kOuterRingExtendRatio = 0.5; // Outer ring extends 50% of radius beyond boundary
constexpr double kBoundaryRingMargin = 0.02;  // Margin for boundary ring to avoid overlap (2% of radius)
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

    // Draw grid if material has Internal grid enabled (radial for circles)
    if (material_model_ && material_model_->grid_type() == MaterialModel::GridType::Internal) {
        // Calculate extended rect for grid (includes outer ring)
        const QRectF baseRect = rect();
        const qreal radius = baseRect.width() / 2.0;
        const qreal extend = radius * kOuterRingExtendRatio;
        const QRectF extendedRect = baseRect.adjusted(-extend, -extend, extend, extend);
        draw_radial_grid(painter, extendedRect, baseRect);
    }
}

void CircleItem::draw_radial_grid(QPainter *painter, const QRectF &extendedRect, const QRectF &baseRect) const {
    if (!material_model_) {
        return;
    }

    painter->save();

    // Remove clipping to allow drawing outside base rect
    painter->setClipping(false);

    // Use composition mode that doesn't darken - draw only lines, no fill
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter->setBrush(Qt::NoBrush);
    QPen gridPen(QColor(0, 0, 0, 255));
    gridPen.setWidthF(0.5);
    painter->setPen(gridPen);

    const QPointF center = baseRect.center();
    const qreal radius = baseRect.width() / 2.0;
    const double freqRadial = material_model_->grid_frequency_x();
    const double freqConcentric = material_model_->grid_frequency_y(); // Used for both inner and outer rings

    // Calculate ring boundaries
    const qreal innerRingStartRadius = radius * kInnerRingStartRatio;
    const qreal innerRingEndRadius = radius;
    const qreal outerRingStartRadius = radius;
    const qreal outerRingEndRadius = radius * (1.0 + kOuterRingExtendRatio);
    const qreal boundaryMargin = radius * kBoundaryRingMargin;

    // Draw concentric circles first (so radial lines appear on top)
    
    // Draw inner ring concentric circles
    const qreal innerRingWidth = innerRingEndRadius - innerRingStartRadius;
    const qreal innerSpacing = innerRingWidth / (freqConcentric + 1); // +1 accounts for boundary circle
    qreal firstInnerRadius = innerRingStartRadius + innerSpacing; // First circle (closest to center)
    qreal currentInnerRadius = firstInnerRadius;
    while (currentInnerRadius < innerRingEndRadius - boundaryMargin) {
        painter->drawEllipse(center, currentInnerRadius, currentInnerRadius);
        currentInnerRadius += innerSpacing;
    }

    // Draw inner ring boundary circle
    painter->drawEllipse(center, innerRingEndRadius, innerRingEndRadius);

    // Draw main boundary circle
    painter->drawEllipse(center, radius, radius);

    // Draw outer ring concentric circles
    const qreal outerRingWidth = outerRingEndRadius - outerRingStartRadius;
    const qreal outerSpacing = outerRingWidth / (freqConcentric + 1); // +1 accounts for boundary circle
    qreal currentOuterRadius = outerRingStartRadius + boundaryMargin + outerSpacing;
    qreal lastOuterRadius = outerRingStartRadius + boundaryMargin; // Last circle (farthest from center)
    while (currentOuterRadius < outerRingEndRadius) {
        painter->drawEllipse(center, currentOuterRadius, currentOuterRadius);
        lastOuterRadius = currentOuterRadius;
        currentOuterRadius += outerSpacing;
    }

    // Draw radial lines (after circles so they appear on top)
    const int numRadialLines = static_cast<int>(freqRadial);
    // Precompute angles to avoid repeated calculations
    QVector<qreal> angles(numRadialLines);
    QVector<qreal> cosValues(numRadialLines);
    QVector<qreal> sinValues(numRadialLines);
    for (int i = 0; i < numRadialLines; ++i) {
        angles[i] = (360.0 * i) / numRadialLines;
        const qreal radians = qDegreesToRadians(angles[i]);
        cosValues[i] = qCos(radians);
        sinValues[i] = qSin(radians);
    }

    for (int i = 0; i < numRadialLines; ++i) {
        const qreal cosA = cosValues[i];
        const qreal sinA = sinValues[i];
        
        // Inner ring: from first circle to boundary
        const QPointF innerStart = center + QPointF(firstInnerRadius * cosA, firstInnerRadius * sinA);
        const QPointF innerEnd = center + QPointF(innerRingEndRadius * cosA, innerRingEndRadius * sinA);
        painter->drawLine(innerStart, innerEnd);
        
        // Outer ring: from boundary to last circle
        const QPointF outerStart = center + QPointF(outerRingStartRadius * cosA, outerRingStartRadius * sinA);
        const QPointF outerEnd = center + QPointF(lastOuterRadius * cosA, lastOuterRadius * sinA);
        painter->drawLine(outerStart, outerEnd);
    }

    painter->restore();
}

QRectF CircleItem::boundingRect() const {
    QRectF baseRect = QGraphicsEllipseItem::boundingRect();
    // Extend bounding rect to include outer grid ring
    // Use the actual rect() to get the circle's radius
    const qreal radius = rect().width() / 2.0;
    const qreal extend = radius * kOuterRingExtendRatio;
    return baseRect.adjusted(-extend, -extend, extend, extend);
}

QVariant CircleItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
        notify_geometry_changed();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

