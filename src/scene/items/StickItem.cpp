#include "StickItem.h"

#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPen>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <cmath>
#include "model/MaterialModel.h"

namespace {
constexpr double kMinLengthPx = 1.0;
constexpr double kMaxLengthPx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
}

StickItem::StickItem(const QLineF &line, QGraphicsItem *parent)
    : QGraphicsLineItem(line, parent) {
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemSendsGeometryChanges);
    QPen p(Qt::black);
    p.setWidthF(2.0);
    setPen(p);
    setTransformOriginPoint(boundingRect().center());
}

void StickItem::set_name(const QString &name) {
    QString trimmed = name.trimmed();
    if (!trimmed.isEmpty()) {
        name_ = trimmed;
    }
}

QWidget* StickItem::create_properties_widget(QWidget *parent) {
    auto *widget = new QWidget(parent);
    auto *form = new QFormLayout(widget);
    form->setContentsMargins(0, 0, 0, 0);

    auto *lengthSpin = new QDoubleSpinBox(widget);
    lengthSpin->setRange(kMinLengthPx, kMaxLengthPx);
    lengthSpin->setDecimals(1);
    QLineF l = line();
    qreal len = std::sqrt(l.dx()*l.dx() + l.dy()*l.dy());
    lengthSpin->setValue(len);
    QObject::connect(lengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget, [this, lengthSpin]{
        QLineF l = line();
        qreal oldLen = std::sqrt(l.dx()*l.dx() + l.dy()*l.dy());
        if (oldLen > 0) {
            qreal scale = lengthSpin->value() / oldLen;
            l.setP2(QPointF(l.x1() + l.dx()*scale, l.y1() + l.dy()*scale));
            setLine(l);
            setTransformOriginPoint(boundingRect().center());
            notify_geometry_changed();
        }
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

    form->addRow("Length:", lengthSpin);
    form->addRow("Rotation:", rotationSpin);

    return widget;
}

QJsonObject StickItem::to_json() const {
    QJsonObject obj;
    obj["type"] = type_name();
    obj["name"] = name_;
    obj["position"] = QJsonArray{pos().x(), pos().y()};
    obj["rotation"] = rotation();
    QLineF l = line();
    obj["line"] = QJsonObject{
        {"x1", l.x1()}, {"y1", l.y1()},
        {"x2", l.x2()}, {"y2", l.y2()}
    };
    QColor c = pen().color();
    obj["pen_color"] = QJsonArray{c.red(), c.green(), c.blue(), c.alpha()};
    obj["pen_width"] = pen().widthF();
    return obj;
}

void StickItem::from_json(const QJsonObject &json) {
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
    if (json.contains("line")) {
        QJsonObject l = json["line"].toObject();
        setLine(QLineF(l["x1"].toDouble(), l["y1"].toDouble(),
                       l["x2"].toDouble(), l["y2"].toDouble()));
        setTransformOriginPoint(boundingRect().center());
    }
    if (json.contains("pen_color")) {
        QJsonArray c = json["pen_color"].toArray();
        QPen p = pen();
        p.setColor(QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt()));
        if (json.contains("pen_width")) {
            p.setWidthF(json["pen_width"].toDouble());
        }
        setPen(p);
    }
}

void StickItem::set_geometry_changed_callback(std::function<void()> callback) {
    geometry_changed_callback_ = std::move(callback);
}

void StickItem::clear_geometry_changed_callback() {
    geometry_changed_callback_ = nullptr;
}

void StickItem::notify_geometry_changed() const {
    if (geometry_changed_callback_) {
        geometry_changed_callback_();
    }
}

void StickItem::set_material_model(MaterialModel *material) {
    // Stick items don't use grid, but we need to implement the interface
    (void)material;
}

QVariant StickItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
        notify_geometry_changed();
    }
    return QGraphicsLineItem::itemChange(change, value);
}

