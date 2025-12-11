#include "RectangleItem.h"

#include <QBrush>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <cmath>

#include "model/MaterialModel.h"

namespace {
constexpr double kMinSizePx = 1.0;
constexpr double kMaxSizePx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
constexpr int kDefaultColorR = 128;
constexpr int kDefaultColorG = 128;
constexpr int kDefaultColorB = 128;
constexpr int kDefaultColorA = 128;
constexpr double kRotationSpinStep = 5.0;
constexpr int kGridPenAlpha = 255;
constexpr double kGridPenWidth = 0.5;
}  // namespace

RectangleItem::RectangleItem(const QRectF& rect, QGraphicsItem* parent)
    : BaseShapeItem<QGraphicsRectItem>(rect, parent) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);
  setPen(QPen(Qt::black, 1.0));
  setBrush(QBrush(
    QColor(kDefaultColorR, kDefaultColorG, kDefaultColorB, kDefaultColorA)));
  setTransformOriginPoint(boundingRect().center());
  // Set default name
  set_name("Rectangle");
}

QWidget* RectangleItem::create_properties_widget(QWidget* parent) {
  auto* widget = new QWidget(parent);
  auto* form = new QFormLayout(widget);
  form->setContentsMargins(0, 0, 0, 0);

  auto* width_spin = new QDoubleSpinBox(widget);
  width_spin->setRange(kMinSizePx, kMaxSizePx);
  width_spin->setDecimals(1);
  width_spin->setValue(rect().width());
  QObject::connect(width_spin,
                   QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
                   [this, width_spin] {
                     const double new_width = width_spin->value();
                     if (new_width < kMinSizePx || new_width > kMaxSizePx) {
                       return;  // Invalid size, ignore
                     }
                     QRectF rect_item = rect();
                     rect_item.setWidth(new_width);
                     setRect(rect_item);
                     setTransformOriginPoint(boundingRect().center());
                     notify_geometry_changed();
                   });

  auto* height_spin = new QDoubleSpinBox(widget);
  height_spin->setRange(kMinSizePx, kMaxSizePx);
  height_spin->setDecimals(1);
  height_spin->setValue(rect().height());
  QObject::connect(height_spin,
                   QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
                   [this, height_spin] {
                     const double new_height = height_spin->value();
                     if (new_height < kMinSizePx || new_height > kMaxSizePx) {
                       return;  // Invalid size, ignore
                     }
                     QRectF rect_item = rect();
                     rect_item.setHeight(new_height);
                     setRect(rect_item);
                     setTransformOriginPoint(boundingRect().center());
                     notify_geometry_changed();
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

  form->addRow("Width:", width_spin);
  form->addRow("Height:", height_spin);
  form->addRow("Rotation:", rotation_spin);

  return widget;
}

QJsonObject RectangleItem::to_json() const {
  QJsonObject obj;
  obj["type"] = type_name();
  obj["name"] = name();
  obj["position"] = QJsonArray{pos().x(), pos().y()};
  obj["rotation"] = rotation();
  obj["width"] = rect().width();
  obj["height"] = rect().height();
  const QColor color = brush().color();
  obj["fill_color"] =
    QJsonArray{color.red(), color.green(), color.blue(), color.alpha()};
  return obj;
}

void RectangleItem::from_json(const QJsonObject& json) {
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
  if (json.contains("width") && json.contains("height")) {
    const double width = json["width"].toDouble();
    const double height = json["height"].toDouble();
    if (std::isfinite(width) && std::isfinite(height) && width > 0.0 &&
        height > 0.0) {
      QRectF rect_item = rect();
      rect_item.setWidth(width);
      rect_item.setHeight(height);
      setRect(rect_item);
      setTransformOriginPoint(boundingRect().center());
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
        setBrush(QBrush(QColor(red, green, blue, alpha)));
      }
    }
  }
}

// set_geometry_changed_callback, clear_geometry_changed_callback,
// notify_geometry_changed, set_material_model are now in BaseShapeItem

void RectangleItem::paint(QPainter* painter,
                          const QStyleOptionGraphicsItem* option,
                          QWidget* widget) {
  // Draw the base rectangle first
  QGraphicsRectItem::paint(painter, option, widget);

  // Draw grid if material has Internal grid enabled
  if (material_model() != nullptr &&
      material_model()->grid_type() == MaterialModel::GridType::Internal) {
    draw_internal_grid(painter, rect());
  }
}

void RectangleItem::draw_internal_grid(QPainter* painter,
                                       const QRectF& rect) const {
  // material_model() is already checked in paint() before calling this
  auto* material = material_model();
  if (material == nullptr) {
    return;
  }
  painter->save();

  // Draw only lines, no fill
  painter->setBrush(Qt::NoBrush);
  QPen grid_pen(QColor(0, 0, 0, kGridPenAlpha));  // Black lines
  grid_pen.setWidthF(kGridPenWidth);
  painter->setPen(grid_pen);

  const double freq_x = material->grid_frequency_x();  // Horizontal cells
  const double freq_y = material->grid_frequency_y();  // Vertical cells

  // Calculate spacing for horizontal and vertical lines separately
  const qreal spacing_x = rect.width() / freq_x;
  const qreal spacing_y = rect.height() / freq_y;

  // Draw vertical lines (horizontal spacing)
  qreal x_pos = rect.left() + spacing_x;
  while (x_pos <= rect.right()) {
    painter->drawLine(QPointF(x_pos, rect.top()),
                      QPointF(x_pos, rect.bottom()));
    x_pos += spacing_x;
  }

  // Draw horizontal lines (vertical spacing)
  qreal y_pos = rect.top() + spacing_y;
  while (y_pos <= rect.bottom()) {
    painter->drawLine(QPointF(rect.left(), y_pos),
                      QPointF(rect.right(), y_pos));
    y_pos += spacing_y;
  }

  painter->restore();
}

QVariant RectangleItem::itemChange(GraphicsItemChange change,
                                   const QVariant& value) {
  handle_geometry_change(change);
  return QGraphicsRectItem::itemChange(change, value);
}
