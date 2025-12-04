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
}  // namespace

RectangleItem::RectangleItem(const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsRectItem(rect, parent) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);
  setPen(QPen(Qt::black, 1.0));
  setBrush(QBrush(QColor(128, 128, 128, 128)));
  setTransformOriginPoint(boundingRect().center());
}

void RectangleItem::set_name(const QString& name) {
  QString trimmed = name.trimmed();
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
                     QRectF r = rect();
                     r.setWidth(widthSpin->value());
                     setRect(r);
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
                     QRectF r = rect();
                     r.setHeight(heightSpin->value());
                     setRect(r);
                     setTransformOriginPoint(boundingRect().center());
                     notify_geometry_changed();
                   });

  auto* rotationSpin = new QDoubleSpinBox(widget);
  rotationSpin->setRange(kMinRotationDeg, kMaxRotationDeg);
  rotationSpin->setDecimals(1);
  rotationSpin->setSingleStep(5.0);
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
  QColor c = brush().color();
  obj["fill_color"] = QJsonArray{c.red(), c.green(), c.blue(), c.alpha()};
  return obj;
}

void RectangleItem::from_json(const QJsonObject& json) {
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
  if (json.contains("width") && json.contains("height")) {
    QRectF r = rect();
    r.setWidth(json["width"].toDouble());
    r.setHeight(json["height"].toDouble());
    setRect(r);
    setTransformOriginPoint(boundingRect().center());
  }
  if (json.contains("fill_color")) {
    QJsonArray c = json["fill_color"].toArray();
    setBrush(
      QBrush(QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt())));
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
  if (material_model_ &&
      material_model_->grid_type() == MaterialModel::GridType::Internal) {
    draw_internal_grid(painter, rect());
  }
}

void RectangleItem::draw_internal_grid(QPainter* painter,
                                       const QRectF& rect) const {
  if (!material_model_) {
    return;
  }

  painter->save();

  // Draw only lines, no fill
  painter->setBrush(Qt::NoBrush);
  QPen gridPen(QColor(0, 0, 0, 255));  // Black lines
  gridPen.setWidthF(0.5);
  painter->setPen(gridPen);

  const double freqX = material_model_->grid_frequency_x();  // Horizontal cells
  const double freqY = material_model_->grid_frequency_y();  // Vertical cells

  // Calculate spacing for horizontal and vertical lines separately
  const qreal spacingX = rect.width() / freqX;
  const qreal spacingY = rect.height() / freqY;

  // Draw vertical lines (horizontal spacing)
  qreal x = rect.left() + spacingX;
  while (x <= rect.right()) {
    painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    x += spacingX;
  }

  // Draw horizontal lines (vertical spacing)
  qreal y = rect.top() + spacingY;
  while (y <= rect.bottom()) {
    painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    y += spacingY;
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
