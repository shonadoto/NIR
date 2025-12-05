#include "EllipseItem.h"

#include <QBrush>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QVariant>
#include <QtMath>

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
constexpr double kFullCircleDegrees = 360.0;
// Grid ring parameters (as fraction of radius)
constexpr double kInnerRingStartRatio =
  0.5;  // Inner ring starts at 50% of radius
constexpr double kOuterRingExtendRatio =
  0.5;  // Outer ring extends 50% of radius beyond boundary
constexpr double kBoundaryRingMargin =
  0.02;  // Margin for boundary ring to avoid overlap (2% of radius)
}  // namespace

EllipseItem::EllipseItem(const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsEllipseItem(rect, parent) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);
  setPen(QPen(Qt::black, 1.0));
  setBrush(QBrush(
    QColor(kDefaultColorR, kDefaultColorG, kDefaultColorB, kDefaultColorA)));
  setTransformOriginPoint(boundingRect().center());
}

void EllipseItem::set_name(const QString& name) {
  const QString trimmed = name.trimmed();
  if (!trimmed.isEmpty()) {
    name_ = trimmed;
  }
}

QWidget* EllipseItem::create_properties_widget(QWidget* parent) {
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

QJsonObject EllipseItem::to_json() const {
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

void EllipseItem::from_json(const QJsonObject& json) {
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

void EllipseItem::set_geometry_changed_callback(
  std::function<void()> callback) {
  geometry_changed_callback_ = std::move(callback);
}

void EllipseItem::clear_geometry_changed_callback() {
  geometry_changed_callback_ = nullptr;
}

void EllipseItem::notify_geometry_changed() const {
  if (geometry_changed_callback_) {
    geometry_changed_callback_();
  }
}

void EllipseItem::set_material_model(MaterialModel* material) {
  material_model_ = material;
  update();  // Trigger repaint to show/hide grid
}

void EllipseItem::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget) {
  // Draw the base ellipse first
  QGraphicsEllipseItem::paint(painter, option, widget);

  // Draw grid if material has Internal grid enabled (radial for ellipses)
  if (material_model_ != nullptr &&
      material_model_->grid_type() == MaterialModel::GridType::Internal) {
    // Calculate extended rect for grid (includes outer ring)
    const QRectF baseRect = rect();
    const qreal maxRadius = qMax(baseRect.width(), baseRect.height()) / 2.0;
    const qreal extend = maxRadius * kOuterRingExtendRatio;
    const QRectF extendedRect =
      baseRect.adjusted(-extend, -extend, extend, extend);
    draw_radial_grid(painter, extendedRect, baseRect);
  }
}

void EllipseItem::draw_radial_grid(QPainter* painter,
                                   const QRectF& /* extendedRect */,
                                   const QRectF& baseRect) const {
  if (material_model_ == nullptr) {
    return;
  }

  painter->save();

  // Remove clipping to allow drawing outside base rect
  painter->setClipping(false);

  // Use composition mode that doesn't darken - draw only lines, no fill
  painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
  painter->setBrush(Qt::NoBrush);
  QPen gridPen(QColor(0, 0, 0, kGridPenAlpha));
  gridPen.setWidthF(kGridPenWidth);
  painter->setPen(gridPen);

  const QPointF center = baseRect.center();
  const qreal halfWidth = baseRect.width() / 2.0;
  const qreal halfHeight = baseRect.height() / 2.0;
  const qreal maxRadius = qMax(halfWidth, halfHeight);
  const double freqRadial = material_model_->grid_frequency_x();
  const double freqConcentric =
    material_model_->grid_frequency_y();  // Used for both inner and outer rings

  // Calculate ring boundaries (using maxRadius as reference)
  const qreal innerRingStartRadius = maxRadius * kInnerRingStartRatio;
  const qreal innerRingEndRadius = maxRadius;
  const qreal outerRingStartRadius = maxRadius;
  const qreal outerRingEndRadius = maxRadius * (1.0 + kOuterRingExtendRatio);
  const qreal boundaryMargin = maxRadius * kBoundaryRingMargin;

  // Helper function to calculate point on ellipse at given scale
  auto ellipsePoint = [&](qreal scale, qreal angle) -> QPointF {
    const qreal radians = qDegreesToRadians(angle);
    const qreal cosA = qCos(radians);
    const qreal sinA = qSin(radians);
    const qreal ellipse_a = halfWidth * scale;
    const qreal ellipse_b = halfHeight * scale;
    const qreal radius =
      (ellipse_a * ellipse_b) / qSqrt(ellipse_b * ellipse_b * cosA * cosA +
                                      ellipse_a * ellipse_a * sinA * sinA);
    return center + QPointF(radius * cosA, radius * sinA);
  };

  // Draw concentric ellipses first (so radial lines appear on top)

  // Draw inner ring concentric ellipses
  const qreal innerRingWidth = innerRingEndRadius - innerRingStartRadius;
  const qreal innerSpacing =
    innerRingWidth / (freqConcentric + 1);  // +1 accounts for boundary ellipse
  const qreal firstInnerRadius =
    innerRingStartRadius + innerSpacing;  // First ellipse (closest to center)
  qreal currentInnerRadius = firstInnerRadius;
  while (currentInnerRadius < innerRingEndRadius - boundaryMargin) {
    const qreal scale = currentInnerRadius / maxRadius;
    const QRectF ellipseRect(center.x() - halfWidth * scale,
                             center.y() - halfHeight * scale,
                             halfWidth * scale * 2, halfHeight * scale * 2);
    painter->drawEllipse(ellipseRect);
    currentInnerRadius += innerSpacing;
  }

  // Draw inner ring boundary ellipse
  const qreal innerBoundaryScale = innerRingEndRadius / maxRadius;
  const QRectF innerBoundaryRect(center.x() - halfWidth * innerBoundaryScale,
                                 center.y() - halfHeight * innerBoundaryScale,
                                 halfWidth * innerBoundaryScale * 2,
                                 halfHeight * innerBoundaryScale * 2);
  painter->drawEllipse(innerBoundaryRect);

  // Draw main boundary ellipse
  painter->drawEllipse(baseRect);

  // Draw outer ring concentric ellipses
  const qreal outerRingWidth = outerRingEndRadius - outerRingStartRadius;
  const qreal outerSpacing =
    outerRingWidth / (freqConcentric + 1);  // +1 accounts for boundary ellipse
  qreal currentOuterRadius =
    outerRingStartRadius + boundaryMargin + outerSpacing;
  qreal lastOuterRadius =
    outerRingStartRadius +
    boundaryMargin;  // Last ellipse (farthest from center)
  while (currentOuterRadius < outerRingEndRadius) {
    const qreal scale = currentOuterRadius / maxRadius;
    const QRectF ellipseRect(center.x() - halfWidth * scale,
                             center.y() - halfHeight * scale,
                             halfWidth * scale * 2, halfHeight * scale * 2);
    painter->drawEllipse(ellipseRect);
    lastOuterRadius = currentOuterRadius;
    currentOuterRadius += outerSpacing;
  }

  // Draw radial lines (after ellipses so they appear on top)
  const int numRadialLines = static_cast<int>(freqRadial);
  // Precompute angles to avoid repeated calculations
  QVector<qreal> angles(numRadialLines);
  for (int i = 0; i < numRadialLines; ++i) {
    angles[i] = (kFullCircleDegrees * i) / numRadialLines;
  }

  for (int i = 0; i < numRadialLines; ++i) {
    const qreal angle = angles[i];

    // Inner ring: from first ellipse to boundary
    const qreal firstInnerScale = firstInnerRadius / maxRadius;
    const qreal innerEndScale = innerRingEndRadius / maxRadius;
    const QPointF innerStart = ellipsePoint(firstInnerScale, angle);
    const QPointF innerEnd = ellipsePoint(innerEndScale, angle);
    painter->drawLine(innerStart, innerEnd);

    // Outer ring: from boundary to last ellipse
    const qreal outerStartScale = outerRingStartRadius / maxRadius;
    const qreal lastOuterScale = lastOuterRadius / maxRadius;
    const QPointF outerStart = ellipsePoint(outerStartScale, angle);
    const QPointF outerEnd = ellipsePoint(lastOuterScale, angle);
    painter->drawLine(outerStart, outerEnd);
  }

  painter->restore();
}

QRectF EllipseItem::boundingRect() const {
  const QRectF baseRect = QGraphicsEllipseItem::boundingRect();
  // Extend bounding rect to include outer grid ring
  // Use the actual rect() to get the ellipse's dimensions
  const qreal maxRadius = qMax(rect().width(), rect().height()) / 2.0;
  const qreal extend = maxRadius * kOuterRingExtendRatio;
  return baseRect.adjusted(-extend, -extend, extend, extend);
}

QVariant EllipseItem::itemChange(GraphicsItemChange change,
                                 const QVariant& value) {
  if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
    notify_geometry_changed();
  }
  return QGraphicsEllipseItem::itemChange(change, value);
}
