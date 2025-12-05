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
    : QGraphicsRectItem(rect, parent) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);
  setPen(QPen(Qt::black, 1.0));
  setBrush(QBrush(
    QColor(kDefaultColorR, kDefaultColorG, kDefaultColorB, kDefaultColorA)));
  setTransformOriginPoint(boundingRect().center());
}

void RectangleItem::set_name(const QString& name) {
  const QString trimmed = name.trimmed();
  if (!trimmed.isEmpty()) {
    name_ = trimmed;
  }
}

QWidget* RectangleItem::create_properties_widget(QWidget* parent) {
  auto* widget = new QWidget(parent);
  auto* form = new QFormLayout(widget);
  form->setContentsMargins(0, 0, 0, 0);

  auto* widthSpin = new QDoubleSpinBox(widget);
  widthSpin->setRange(kMinSizePx, kMaxSizePx);
  widthSpin->setDecimals(1);
  widthSpin->setValue(rect().width());
  QObject::connect(widthSpin,
                   QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
                   [this, widthSpin] {
                     QRectF rect_item = rect();
                     rect_item.setWidth(widthSpin->value());
                     setRect(rect_item);
                     setTransformOriginPoint(boundingRect().center());
                     notify_geometry_changed();
                   });

  auto* heightSpin = new QDoubleSpinBox(widget);
  heightSpin->setRange(kMinSizePx, kMaxSizePx);
  heightSpin->setDecimals(1);
  heightSpin->setValue(rect().height());
  QObject::connect(heightSpin,
                   QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
                   [this, heightSpin] {
                     QRectF rect_item = rect();
                     rect_item.setHeight(heightSpin->value());
                     setRect(rect_item);
                     setTransformOriginPoint(boundingRect().center());
                     notify_geometry_changed();
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

  form->addRow("Width:", widthSpin);
  form->addRow("Height:", heightSpin);
  form->addRow("Rotation:", rotationSpin);

  return widget;
}

QJsonObject RectangleItem::to_json() const {
  QJsonObject obj;
  obj["type"] = type_name();
  obj["name"] = name_;
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
    name_ = json["name"].toString();
  }
  if (json.contains("position")) {
    QJsonArray position_array = json["position"].toArray();
    setPos(position_array[0].toDouble(), position_array[1].toDouble());
  }
  if (json.contains("rotation")) {
    setRotation(json["rotation"].toDouble());
  }
  if (json.contains("width") && json.contains("height")) {
    QRectF rect_item = rect();
    rect_item.setWidth(json["width"].toDouble());
    rect_item.setHeight(json["height"].toDouble());
    setRect(rect_item);
    setTransformOriginPoint(boundingRect().center());
  }
  if (json.contains("fill_color")) {
    QJsonArray color_array = json["fill_color"].toArray();
    setBrush(QBrush(QColor(color_array[0].toInt(), color_array[1].toInt(),
                           color_array[2].toInt(), color_array[3].toInt())));
  }
}

void RectangleItem::set_geometry_changed_callback(
  std::function<void()> callback) {
  geometry_changed_callback_ = std::move(callback);
}

void RectangleItem::clear_geometry_changed_callback() {
  geometry_changed_callback_ = nullptr;
}

void RectangleItem::notify_geometry_changed() const {
  if (geometry_changed_callback_) {
    geometry_changed_callback_();
  }
}

void RectangleItem::set_material_model(MaterialModel* material) {
  material_model_ = material;
  update();  // Trigger repaint to show/hide grid
}

void RectangleItem::paint(QPainter* painter,
                          const QStyleOptionGraphicsItem* option,
                          QWidget* widget) {
  // Draw the base rectangle first
  QGraphicsRectItem::paint(painter, option, widget);

  // Draw grid if material has Internal grid enabled
  if (material_model_ != nullptr &&
      material_model_->grid_type() == MaterialModel::GridType::Internal) {
    draw_internal_grid(painter, rect());
  }
}

void RectangleItem::draw_internal_grid(QPainter* painter,
                                       const QRectF& rect) const {
  if (material_model_ == nullptr) {
    return;
  }

  painter->save();

  // Draw only lines, no fill
  painter->setBrush(Qt::NoBrush);
  QPen gridPen(QColor(0, 0, 0, kGridPenAlpha));  // Black lines
  gridPen.setWidthF(kGridPenWidth);
  painter->setPen(gridPen);

  const double freqX = material_model_->grid_frequency_x();  // Horizontal cells
  const double freqY = material_model_->grid_frequency_y();  // Vertical cells

  // Calculate spacing for horizontal and vertical lines separately
  const qreal spacingX = rect.width() / freqX;
  const qreal spacingY = rect.height() / freqY;

  // Draw vertical lines (horizontal spacing)
  qreal x_pos = rect.left() + spacingX;
  while (x_pos <= rect.right()) {
    painter->drawLine(QPointF(x_pos, rect.top()),
                      QPointF(x_pos, rect.bottom()));
    x_pos += spacingX;
  }

  // Draw horizontal lines (vertical spacing)
  qreal y_pos = rect.top() + spacingY;
  while (y_pos <= rect.bottom()) {
    painter->drawLine(QPointF(rect.left(), y_pos),
                      QPointF(rect.right(), y_pos));
    y_pos += spacingY;
  }

  painter->restore();
}

QVariant RectangleItem::itemChange(GraphicsItemChange change,
                                   const QVariant& value) {
  if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
    notify_geometry_changed();
  }
  return QGraphicsRectItem::itemChange(change, value);
}
