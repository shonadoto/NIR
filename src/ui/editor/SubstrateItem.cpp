#include "SubstrateItem.h"

#include <QColorDialog>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QPushButton>
#include <cmath>

namespace {
constexpr qreal kOutlineWidthPx = 1.0;
constexpr int kSubstratePenColorR = 180;
constexpr int kSubstratePenColorG = 180;
constexpr int kSubstratePenColorB = 180;
}  // namespace

SubstrateItem::SubstrateItem(const QSizeF& size) : size_(size) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemIsMovable, false);
  setZValue(-100.0);  // stay behind other shapes later
}

void SubstrateItem::set_name(const QString& name) {
  const QString trimmed = name.trimmed();
  if (trimmed.isEmpty() || trimmed == name_) {
    return;
  }
  name_ = trimmed;
}

auto SubstrateItem::boundingRect() const -> QRectF {
  return {QPointF(0, 0), size_};
}

void SubstrateItem::paint(QPainter* painter,
                          const QStyleOptionGraphicsItem* /*option*/,
                          QWidget* /*widget*/) {
  painter->setRenderHint(QPainter::Antialiasing, true);
  const QRectF rect = boundingRect().adjusted(
    kOutlineWidthPx, kOutlineWidthPx, -kOutlineWidthPx, -kOutlineWidthPx);

  // Fill
  painter->fillRect(rect, fill_color_);

  // Outline
  QPen pen(
    QColor(kSubstratePenColorR, kSubstratePenColorG, kSubstratePenColorB));
  pen.setWidthF(kOutlineWidthPx);
  painter->setPen(pen);
  painter->drawRect(rect);
}

void SubstrateItem::set_size(const QSizeF& size) {
  if (size == size_) {
    return;
  }
  // Validate size: both dimensions must be positive and finite
  if (size.width() <= 0.0 || size.height() <= 0.0 ||
      !std::isfinite(size.width()) || !std::isfinite(size.height())) {
    return;  // Invalid size, ignore
  }
  prepareGeometryChange();
  size_ = size;
}

void SubstrateItem::set_fill_color(const QColor& color) {
  fill_color_ = color;
  update();
}

auto SubstrateItem::create_properties_widget(QWidget* parent) -> QWidget* {
  auto* widget = new QWidget(parent);
  auto* form = new QFormLayout(widget);
  form->setContentsMargins(0, 0, 0, 0);

  auto* color_btn = new QPushButton("Choose Color", widget);
  QObject::connect(color_btn, &QPushButton::clicked, widget, [this, color_btn] {
    const QColor color =
      QColorDialog::getColor(fill_color_, color_btn, "Choose Substrate Color");
    if (color.isValid()) {
      set_fill_color(color);
    }
  });

  form->addRow("Color:", color_btn);

  return widget;
}

QJsonObject SubstrateItem::to_json() const {
  QJsonObject obj;
  obj["type"] = type_name();
  obj["name"] = name_;
  obj["width"] = size_.width();
  obj["height"] = size_.height();
  obj["fill_color"] = QJsonArray{fill_color_.red(), fill_color_.green(),
                                 fill_color_.blue(), fill_color_.alpha()};
  return obj;
}

void SubstrateItem::from_json(const QJsonObject& json) {
  if (json.contains("name")) {
    const QString name = json["name"].toString();
    if (!name.isEmpty()) {
      name_ = name;
    }
  }
  if (json.contains("width") && json.contains("height")) {
    const double width = json["width"].toDouble();
    const double height = json["height"].toDouble();
    if (std::isfinite(width) && std::isfinite(height) && width > 0.0 &&
        height > 0.0) {
      set_size(QSizeF(width, height));
    }
  }
  if (json.contains("fill_color")) {
    QJsonArray color_array = json["fill_color"].toArray();
    if (color_array.size() >= 4) {
      constexpr int kMaxColorValue = 255;
      const int red = color_array[0].toInt();
      const int green = color_array[1].toInt();
      const int blue = color_array[2].toInt();
      const int alpha = color_array[3].toInt();
      // Validate color values are in valid range [0, 255]
      if (red >= 0 && red <= kMaxColorValue && green >= 0 &&
          green <= kMaxColorValue && blue >= 0 && blue <= kMaxColorValue &&
          alpha >= 0 && alpha <= kMaxColorValue) {
        set_fill_color(QColor(red, green, blue, alpha));
      }
    }
  }
}
