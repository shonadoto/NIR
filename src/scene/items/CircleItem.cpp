#include "CircleItem.h"

#include <QBrush>
#include <QColor>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPen>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>

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
    });

    auto *rotationSpin = new QDoubleSpinBox(widget);
    rotationSpin->setRange(kMinRotationDeg, kMaxRotationDeg);
    rotationSpin->setDecimals(1);
    rotationSpin->setSingleStep(5.0);
    rotationSpin->setSuffix("°");
    rotationSpin->setValue(rotation());
    QObject::connect(rotationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget, [this, rotationSpin]{
        setRotation(rotationSpin->value());
    });

    auto *colorBtn = new QPushButton("Choose Color", widget);
    QObject::connect(colorBtn, &QPushButton::clicked, widget, [this, colorBtn]{
        QColor c = QColorDialog::getColor(brush().color(), colorBtn, "Choose Fill Color", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            setBrush(QBrush(c));
        }
    });

    form->addRow("Radius:", radiusSpin);
    form->addRow("Rotation:", rotationSpin);
    form->addRow("Color:", colorBtn);

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

