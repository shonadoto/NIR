#include "StickItem.h"

#include <QBrush>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QVariant>
#include <cmath>

#include "model/MaterialModel.h"

namespace {
constexpr double kMinLengthPx = 1.0;
constexpr double kMaxLengthPx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
constexpr double kDefaultPenWidth = 2.0;
constexpr double kRotationSpinStep = 5.0;
}  // namespace

StickItem::StickItem(const QLineF& line, QGraphicsItem* parent)
    : BaseShapeItem<QGraphicsLineItem>(line, parent) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);
  QPen pen_item(Qt::black);
  pen_item.setWidthF(kDefaultPenWidth);
  setPen(pen_item);
  setTransformOriginPoint(boundingRect().center());
  // Set default name
  set_name("Stick");
}

QWidget* StickItem::create_properties_widget(QWidget* parent) {
  auto* widget = new QWidget(parent);
  auto* form = new QFormLayout(widget);
  form->setContentsMargins(0, 0, 0, 0);

  auto* length_spin = new QDoubleSpinBox(widget);
  length_spin->setRange(kMinLengthPx, kMaxLengthPx);
  length_spin->setDecimals(1);
  const QLineF line_item = line();
  const qreal len = std::sqrt(line_item.dx() * line_item.dx() +
                              line_item.dy() * line_item.dy());
  length_spin->setValue(len);
  QObject::connect(
    length_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
    [this, length_spin] {
      const qreal new_length = length_spin->value();
      if (new_length < kMinLengthPx || new_length > kMaxLengthPx) {
        return;  // Invalid length, ignore
      }
      QLineF line_item = line();
      const qreal old_len = std::sqrt(line_item.dx() * line_item.dx() +
                                      line_item.dy() * line_item.dy());
      if (old_len > 0) {
        const qreal scale = new_length / old_len;
        line_item.setP2(QPointF(line_item.x1() + line_item.dx() * scale,
                                line_item.y1() + line_item.dy() * scale));
        setLine(line_item);
        setTransformOriginPoint(boundingRect().center());
        notify_geometry_changed();
      }
    });

  auto* rotation_spin = new QDoubleSpinBox(widget);
  rotation_spin->setRange(kMinRotationDeg, kMaxRotationDeg);
  rotation_spin->setDecimals(1);
  rotation_spin->setSingleStep(kRotationSpinStep);
  rotation_spin->setSuffix("°");
  rotation_spin->setValue(rotation());
  QObject::connect(rotation_spin,
                   QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
                   [this, rotation_spin] {
                     setRotation(rotation_spin->value());
                     notify_geometry_changed();
                   });

  form->addRow("Length:", length_spin);
  form->addRow("Rotation:", rotation_spin);

  return widget;
}

QJsonObject StickItem::to_json() const {
  QJsonObject obj;
  obj["type"] = type_name();
  obj["name"] = name();
  obj["position"] = QJsonArray{pos().x(), pos().y()};
  obj["rotation"] = rotation();
  const QLineF line_item = line();
  obj["line"] = QJsonObject{{"x1", line_item.x1()},
                            {"y1", line_item.y1()},
                            {"x2", line_item.x2()},
                            {"y2", line_item.y2()}};
  const QColor color = pen().color();
  obj["pen_color"] =
    QJsonArray{color.red(), color.green(), color.blue(), color.alpha()};
  obj["pen_width"] = pen().widthF();
  return obj;
}

void StickItem::from_json(const QJsonObject& json) {
  if (json.contains("name")) {
    const QString name = json["name"].toString();
    if (!name.isEmpty()) {
      set_name(name);
    }
  }
  if (json.contains("position")) {
    QJsonArray position_array = json["position"].toArray();
    if (position_array.size() >= 2) {
      const double pos_x = position_array[0].toDouble();
      const double pos_y = position_array[1].toDouble();
      if (std::isfinite(pos_x) && std::isfinite(pos_y)) {
        setPos(pos_x, pos_y);
      }
    }
  }
  if (json.contains("rotation")) {
    const double rot = json["rotation"].toDouble();
    if (std::isfinite(rot)) {
      setRotation(rot);
    }
  }
  if (json.contains("line")) {
    QJsonObject line_obj = json["line"].toObject();
    if (line_obj.contains("x1") && line_obj.contains("y1") &&
        line_obj.contains("x2") && line_obj.contains("y2")) {
      const double line_x1 = line_obj["x1"].toDouble();
      const double line_y1 = line_obj["y1"].toDouble();
      const double line_x2 = line_obj["x2"].toDouble();
      const double line_y2 = line_obj["y2"].toDouble();
      if (std::isfinite(line_x1) && std::isfinite(line_y1) &&
          std::isfinite(line_x2) && std::isfinite(line_y2)) {
        setLine(QLineF(line_x1, line_y1, line_x2, line_y2));
        setTransformOriginPoint(boundingRect().center());
      }
    }
  }
  if (json.contains("pen_color")) {
    QJsonArray color_array = json["pen_color"].toArray();
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
        QPen pen_item = pen();
        pen_item.setColor(QColor(red, green, blue, alpha));
        if (json.contains("pen_width")) {
          const double width = json["pen_width"].toDouble();
          if (std::isfinite(width) && width > 0.0) {
            pen_item.setWidthF(width);
          }
        }
        setPen(pen_item);
      }
    }
  }
}

// set_geometry_changed_callback, clear_geometry_changed_callback,
// notify_geometry_changed, set_material_model are now in BaseShapeItem

QRectF StickItem::boundingRect() const {
  // Return bounding rect that includes only the line with pen width
  QRectF rect = QGraphicsLineItem::boundingRect();
  return rect;
}

void StickItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                      QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  // Draw only the line - no brush, no fill, no background
  painter->save();
  painter->setBrush(Qt::NoBrush);
  painter->setRenderHint(QPainter::Antialiasing, true);

  // Draw the line directly with the pen
  const QPen pen_item = pen();
  painter->setPen(pen_item);
  painter->drawLine(line());

  painter->restore();
}

QVariant StickItem::itemChange(GraphicsItemChange change,
                               const QVariant& value) {
  handle_geometry_change(change);
  return QGraphicsLineItem::itemChange(change, value);
}
