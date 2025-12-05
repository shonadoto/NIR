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
    : QGraphicsLineItem(line, parent) {
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemSendsGeometryChanges);
  QPen pen_item(Qt::black);
  pen_item.setWidthF(kDefaultPenWidth);
  setPen(pen_item);
    setTransformOriginPoint(boundingRect().center());
}

void StickItem::set_name(const QString& name) {
    const QString trimmed = name.trimmed();
    if (!trimmed.isEmpty()) {
        name_ = trimmed;
    }
}

QWidget* StickItem::create_properties_widget(QWidget* parent) {
  auto* widget = new QWidget(parent);
  auto* form = new QFormLayout(widget);
    form->setContentsMargins(0, 0, 0, 0);

  auto* lengthSpin = new QDoubleSpinBox(widget);
    lengthSpin->setRange(kMinLengthPx, kMaxLengthPx);
    lengthSpin->setDecimals(1);
  const QLineF line_item = line();
  const qreal len = std::sqrt(line_item.dx() * line_item.dx() +
                              line_item.dy() * line_item.dy());
    lengthSpin->setValue(len);
  QObject::connect(
    lengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
    [this, lengthSpin] {
      QLineF line_item = line();
      const qreal oldLen = std::sqrt(line_item.dx() * line_item.dx() +
                                     line_item.dy() * line_item.dy());
        if (oldLen > 0) {
            const qreal scale = lengthSpin->value() / oldLen;
        line_item.setP2(QPointF(line_item.x1() + line_item.dx() * scale,
                                line_item.y1() + line_item.dy() * scale));
        setLine(line_item);
            setTransformOriginPoint(boundingRect().center());
        notify_geometry_changed();
        }
    });

  auto* rotationSpin = new QDoubleSpinBox(widget);
    rotationSpin->setRange(kMinRotationDeg, kMaxRotationDeg);
    rotationSpin->setDecimals(1);
  rotationSpin->setSingleStep(kRotationSpinStep);
    rotationSpin->setSuffix("°");
    rotationSpin->setValue(rotation());
  QObject::connect(rotationSpin,
                   QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
                   [this, rotationSpin] {
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
        name_ = json["name"].toString();
    }
    if (json.contains("position")) {
    QJsonArray position_array = json["position"].toArray();
    setPos(position_array[0].toDouble(), position_array[1].toDouble());
    }
    if (json.contains("rotation")) {
        setRotation(json["rotation"].toDouble());
    }
    if (json.contains("line")) {
    QJsonObject line_obj = json["line"].toObject();
    setLine(QLineF(line_obj["x1"].toDouble(), line_obj["y1"].toDouble(),
                   line_obj["x2"].toDouble(), line_obj["y2"].toDouble()));
        setTransformOriginPoint(boundingRect().center());
    }
    if (json.contains("pen_color")) {
    QJsonArray color_array = json["pen_color"].toArray();
    QPen pen_item = pen();
    pen_item.setColor(QColor(color_array[0].toInt(), color_array[1].toInt(),
                             color_array[2].toInt(), color_array[3].toInt()));
        if (json.contains("pen_width")) {
      pen_item.setWidthF(json["pen_width"].toDouble());
        }
    setPen(pen_item);
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

void StickItem::set_material_model(MaterialModel* material) {
  // Stick items don't use grid, but we need to implement the interface
  (void)material;
}

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
  if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
    notify_geometry_changed();
  }
  return QGraphicsLineItem::itemChange(change, value);
}
