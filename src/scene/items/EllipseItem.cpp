#include "EllipseItem.h"

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
constexpr double kMinSizePx = 1.0;
constexpr double kMaxSizePx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
}

EllipseItem::EllipseItem(const QRectF &rect, QGraphicsItem *parent)
    : QGraphicsEllipseItem(rect, parent) {
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setPen(QPen(Qt::black, 1.0));
    setBrush(QBrush(QColor(128, 128, 128, 128)));
    setTransformOriginPoint(boundingRect().center());
}

void EllipseItem::set_name(const QString &name) {
    QString trimmed = name.trimmed();
    if (!trimmed.isEmpty()) {
        name_ = trimmed;
    }
}

QWidget* EllipseItem::create_properties_widget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *form = new QFormLayout(widget);
    form->setContentsMargins(0, 0, 0, 0);

    auto *widthSpin = new QDoubleSpinBox(widget);
    widthSpin->setRange(kMinSizePx, kMaxSizePx);
    widthSpin->setDecimals(1);
    widthSpin->setValue(rect().width());
    QObject::connect(widthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget, [this, widthSpin]{
        QRectF r = rect();
        r.setWidth(widthSpin->value());
        setRect(r);
        setTransformOriginPoint(boundingRect().center());
        notify_geometry_changed();
    });

    auto *heightSpin = new QDoubleSpinBox(widget);
    heightSpin->setRange(kMinSizePx, kMaxSizePx);
    heightSpin->setDecimals(1);
    heightSpin->setValue(rect().height());
    QObject::connect(heightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget, [this, heightSpin]{
        QRectF r = rect();
        r.setHeight(heightSpin->value());
        setRect(r);
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

    form->addRow("Width:", widthSpin);
    form->addRow("Height:", heightSpin);
    form->addRow("Rotation:", rotationSpin);

    return widget;
}

QJsonObject EllipseItem::to_json() const {
    QJsonObject obj;
    obj["type"] = type_name();
    obj["name"] = name_;
    obj["position"] = QJsonArray{pos().x(), pos().y()};
    obj["rotation"] = rotation();
    obj["width"] = rect().width();
    obj["height"] = rect().height();
    QColor c = brush().color();
    obj["fill_color"] = QJsonArray{c.red(), c.green(), c.blue(), c.alpha()};
    return obj;
}

void EllipseItem::from_json(const QJsonObject &json) {
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
    if (json.contains("width") && json.contains("height")) {
        QRectF r = rect();
        r.setWidth(json["width"].toDouble());
        r.setHeight(json["height"].toDouble());
        setRect(r);
        setTransformOriginPoint(boundingRect().center());
    }
    if (json.contains("fill_color")) {
        QJsonArray c = json["fill_color"].toArray();
        setBrush(QBrush(QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt())));
    }
}

void EllipseItem::set_geometry_changed_callback(std::function<void()> callback) {
    geometry_changed_callback_ = std::move(callback);
}

void EllipseItem::clear_geometry_changed_callback() {
    geometry_changed_callback_ = nullptr;
}

void EllipseItem::notify_geometry_changed() const {
    if (geometry_changed_callback_) {
        geometry_changed_callback_();
    }
}

void EllipseItem::set_material_model(MaterialModel *material) {
    material_model_ = material;
    update(); // Trigger repaint to show/hide grid
}

void EllipseItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // Draw the base ellipse first
    QGraphicsEllipseItem::paint(painter, option, widget);

    // Draw grid if material has Internal grid enabled (radial for ellipses)
    if (material_model_ && material_model_->grid_type() == MaterialModel::GridType::Internal) {
        draw_radial_grid(painter, rect());
    }
}

void EllipseItem::draw_radial_grid(QPainter *painter, const QRectF &rect) const {
    if (!material_model_) {
        return;
    }

    painter->save();

    // Clip to ellipse boundary
    QPainterPath clipPath;
    clipPath.addEllipse(rect);
    painter->setClipPath(clipPath);

    QPen gridPen(QColor(0, 0, 0, 100)); // Semi-transparent black
    gridPen.setWidthF(0.5);
    painter->setPen(gridPen);

    const QPointF center = rect.center();
    const qreal halfWidth = rect.width() / 2.0;
    const qreal halfHeight = rect.height() / 2.0;
    const qreal maxRadius = qMax(halfWidth, halfHeight);
    const double freqRadial = material_model_->grid_frequency_x(); // Number of radial lines
    const double freqConcentric = material_model_->grid_frequency_y(); // Number of concentric ellipses

    // Draw radial lines
    const int numRadialLines = static_cast<int>(freqRadial);
    for (int i = 0; i < numRadialLines; ++i) {
        const qreal angle = (360.0 * i) / numRadialLines;
        const qreal radians = qDegreesToRadians(angle);
        // Calculate point on ellipse edge
        const qreal cosA = qCos(radians);
        const qreal sinA = qSin(radians);
        const qreal a = halfWidth;
        const qreal b = halfHeight;
        const qreal r = (a * b) / qSqrt(b * b * cosA * cosA + a * a * sinA * sinA);
        const QPointF endPoint = center + QPointF(r * cosA, r * sinA);
        painter->drawLine(center, endPoint);
    }

    // Draw concentric ellipses
    const qreal spacing = maxRadius / freqConcentric;
    qreal currentScale = spacing / maxRadius;
    while (currentScale < 1.0) {
        const QRectF ellipseRect(
            center.x() - halfWidth * currentScale,
            center.y() - halfHeight * currentScale,
            halfWidth * currentScale * 2,
            halfHeight * currentScale * 2
        );
        painter->drawEllipse(ellipseRect);
        currentScale += spacing / maxRadius;
    }

    painter->restore();
}

QVariant EllipseItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
        notify_geometry_changed();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

