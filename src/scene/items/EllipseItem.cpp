#include "EllipseItem.h"

#include <QBrush>
#include <QColor>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPen>
#include <QPushButton>

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

    form->addRow("Width:", widthSpin);
    form->addRow("Height:", heightSpin);
    form->addRow("Rotation:", rotationSpin);
    form->addRow("Color:", colorBtn);

    return widget;
}

