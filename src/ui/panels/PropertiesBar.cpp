#include "PropertiesBar.h"

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <cmath>

#include "ui/editor/SubstrateItem.h"

namespace {
constexpr int kMinPropertiesBarWidthPx = 220;
constexpr int kDefaultPropertiesBarWidthPx = 280;
constexpr double kMinSizePx = 1.0;
constexpr double kMaxSizePx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
}

PropertiesBar::PropertiesBar(QWidget *parent)
    : QWidget(parent), current_color_(128, 128, 128, 128) {
    create_ui();
    setMinimumWidth(kMinPropertiesBarWidthPx);
    setFixedWidth(kDefaultPropertiesBarWidthPx);
    preferred_width_ = kDefaultPropertiesBarWidthPx;
    clear();
}

PropertiesBar::~PropertiesBar() = default;

void PropertiesBar::create_ui() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);

    auto *title = new QLabel("Properties", this);
    QFont f = title->font();
    f.setBold(true);
    title->setFont(f);
    layout->addWidget(title);

    form_ = new QFormLayout();
    form_->setContentsMargins(0, 4, 0, 0);
    form_->setSpacing(6);

    width_spin_ = new QDoubleSpinBox(this);
    width_spin_->setRange(kMinSizePx, kMaxSizePx);
    width_spin_->setDecimals(1);
    width_spin_->setSingleStep(1.0);
    connect(width_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertiesBar::on_value_changed);

    height_spin_ = new QDoubleSpinBox(this);
    height_spin_->setRange(kMinSizePx, kMaxSizePx);
    height_spin_->setDecimals(1);
    height_spin_->setSingleStep(1.0);
    connect(height_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertiesBar::on_value_changed);

    rotation_spin_ = new QDoubleSpinBox(this);
    rotation_spin_->setRange(kMinRotationDeg, kMaxRotationDeg);
    rotation_spin_->setDecimals(1);
    rotation_spin_->setSingleStep(5.0);
    rotation_spin_->setSuffix("°");
    connect(rotation_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertiesBar::on_value_changed);

    color_button_ = new QPushButton("Choose Color", this);
    connect(color_button_, &QPushButton::clicked, this, [this]{
        QColor c = QColorDialog::getColor(current_color_, this, "Choose Fill Color", QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            current_color_ = c;
            on_value_changed();
        }
    });

    form_->addRow("Width:", width_spin_);
    form_->addRow("Height:", height_spin_);
    form_->addRow("Rotation:", rotation_spin_);
    form_->addRow("Color:", color_button_);

    layout->addLayout(form_);
    layout->addStretch();
}

void PropertiesBar::set_selected_item(QGraphicsItem *item) {
    current_item_ = item;
    update_from_item();
}

void PropertiesBar::clear() {
    current_item_ = nullptr;
    setEnabled(false);
}

void PropertiesBar::update_from_item() {
    if (!current_item_) {
        clear();
        return;
    }
    updating_ = true;
    setEnabled(true);

    QRectF br = current_item_->boundingRect();
    width_spin_->setValue(br.width());
    height_spin_->setValue(br.height());
    rotation_spin_->setValue(current_item_->rotation());

    // Try to get color from brush
    if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(current_item_)) {
        current_color_ = rectItem->brush().color();
    } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(current_item_)) {
        current_color_ = ellipseItem->brush().color();
    } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(current_item_)) {
        QPen linePen = lineItem->pen();
        current_color_ = linePen.color();
    } else if (dynamic_cast<SubstrateItem*>(current_item_)) {
        // Substrate: read-only size
        width_spin_->setEnabled(false);
        height_spin_->setEnabled(false);
        rotation_spin_->setEnabled(false);
        color_button_->setEnabled(false);
    }

    updating_ = false;
}

void PropertiesBar::apply_to_item() {
    if (!current_item_ || updating_) {
        return;
    }

    current_item_->setRotation(rotation_spin_->value());

    // Apply size (rect/ellipse only)
    const qreal w = width_spin_->value();
    const qreal h = height_spin_->value();
    if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(current_item_)) {
        QRectF r = rectItem->rect();
        r.setWidth(w);
        r.setHeight(h);
        rectItem->setRect(r);
        rectItem->setBrush(QBrush(current_color_));
    } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(current_item_)) {
        QRectF r = ellipseItem->rect();
        r.setWidth(w);
        r.setHeight(h);
        ellipseItem->setRect(r);
        ellipseItem->setBrush(QBrush(current_color_));
    } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(current_item_)) {
        QLineF l = lineItem->line();
        qreal len = std::sqrt(l.dx()*l.dx() + l.dy()*l.dy());
        if (len > 0) {
            qreal newLen = w; // use width as length
            qreal scale = newLen / len;
            l.setP2(QPointF(l.x1() + l.dx()*scale, l.y1() + l.dy()*scale));
            lineItem->setLine(l);
        }
        QPen p = lineItem->pen();
        p.setColor(current_color_);
        lineItem->setPen(p);
    }

    emit property_changed();
}

void PropertiesBar::on_value_changed() {
    apply_to_item();
}
