#include "SubstrateItem.h"

#include <QColorDialog>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QPushButton>

namespace {
constexpr qreal kOutlineWidthPx = 1.0;
}

SubstrateItem::SubstrateItem(const QSizeF& size) : size_(size) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemIsMovable, false);
  setZValue(-100.0);  // stay behind other shapes later
}

void SubstrateItem::set_name(const QString& name) {
  QString trimmed = name.trimmed();
  if (!trimmed.isEmpty()) {
    name_ = trimmed;
  }
}

QRectF SubstrateItem::boundingRect() const {
  return QRectF(QPointF(0, 0), size_);
}

void SubstrateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*,
                          QWidget*) {
  painter->setRenderHint(QPainter::Antialiasing, true);
  const QRectF rect = boundingRect().adjusted(
    kOutlineWidthPx, kOutlineWidthPx, -kOutlineWidthPx, -kOutlineWidthPx);

  // Fill
  painter->fillRect(rect, fill_color_);

  // Outline
  QPen pen(QColor(180, 180, 180));
  pen.setWidthF(kOutlineWidthPx);
  painter->setPen(pen);
  painter->drawRect(rect);
}

void SubstrateItem::set_size(const QSizeF& size) {
  if (size == size_) {
    return;
  }
  prepareGeometryChange();
  size_ = size;
}

void SubstrateItem::set_fill_color(const QColor& color) {
  fill_color_ = color;
  update();
}

QWidget* SubstrateItem::create_properties_widget(QWidget* parent) {
  auto* widget = new QWidget(parent);
  auto* form = new QFormLayout(widget);
  form->setContentsMargins(0, 0, 0, 0);

  auto* colorBtn = new QPushButton("Choose Color", widget);
  QObject::connect(colorBtn, &QPushButton::clicked, widget, [this, colorBtn] {
    QColor c =
      QColorDialog::getColor(fill_color_, colorBtn, "Choose Substrate Color");
    if (c.isValid()) {
      set_fill_color(c);
    }
  });

  form->addRow("Color:", colorBtn);

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
    name_ = json["name"].toString();
  }
  if (json.contains("width") && json.contains("height")) {
    set_size(QSizeF(json["width"].toDouble(), json["height"].toDouble()));
  }
  if (json.contains("fill_color")) {
    QJsonArray c = json["fill_color"].toArray();
    set_fill_color(
      QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt()));
  }
}
