#include "StickItem.h"

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPen>
#include <QPushButton>
#include <cmath>

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
    });

    auto *colorBtn = new QPushButton("Choose Color", widget);
    QObject::connect(colorBtn, &QPushButton::clicked, widget, [this, colorBtn]{
        QColor c = QColorDialog::getColor(pen().color(), colorBtn, "Choose Line Color", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            QPen p = pen();
            p.setColor(c);
            setPen(p);
        }
    });

    form->addRow("Length:", lengthSpin);
    form->addRow("Rotation:", rotationSpin);
    form->addRow("Color:", colorBtn);

    return widget;
}

