#include "EllipseItem.h"

#include <QBrush>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPen>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

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

QVariant EllipseItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
        notify_geometry_changed();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

