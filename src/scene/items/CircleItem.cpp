#include "CircleItem.h"

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
#include <cmath>

#include "model/MaterialModel.h"

namespace {
constexpr double kMinRadiusPx = 1.0;
constexpr double kMaxRadiusPx = 10000.0;
constexpr double kMinRotationDeg = -360.0;
constexpr double kMaxRotationDeg = 360.0;
constexpr int kDefaultColorR = 128;
constexpr int kDefaultColorG = 128;
constexpr int kDefaultColorB = 128;
constexpr int kDefaultColorA = 128;
constexpr double kRadiusDivisor = 2.0;
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

CircleItem::CircleItem(qreal radius, QGraphicsItem* parent)
    : BaseShapeItem<QGraphicsEllipseItem>(
        QRectF(-radius, -radius, 2 * radius, 2 * radius), parent) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);
  setPen(QPen(Qt::black, 1.0));
  setBrush(QBrush(
    QColor(kDefaultColorR, kDefaultColorG, kDefaultColorB, kDefaultColorA)));
  setTransformOriginPoint(boundingRect().center());
  // Set default name
  set_name("Circle");
}

QWidget* CircleItem::create_properties_widget(QWidget* parent) {
  auto* widget = new QWidget(parent);
  auto* form = new QFormLayout(widget);
  form->setContentsMargins(0, 0, 0, 0);

  auto* radius_spin = new QDoubleSpinBox(widget);
  radius_spin->setRange(kMinRadiusPx, kMaxRadiusPx);
  radius_spin->setDecimals(1);
  radius_spin->setValue(rect().width() / kRadiusDivisor);
  QObject::connect(
    radius_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), widget,
    [this, radius_spin] {
      const qreal radius_value = radius_spin->value();
      if (radius_value < kMinRadiusPx || radius_value > kMaxRadiusPx) {
        return;  // Invalid radius, ignore
      }
      setRect(QRectF(-radius_value, -radius_value,
                     kRadiusDivisor * radius_value,
                     kRadiusDivisor * radius_value));
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

  form->addRow("Radius:", radius_spin);
  form->addRow("Rotation:", rotation_spin);

  return widget;
}

QJsonObject CircleItem::to_json() const {
  QJsonObject obj;
  obj["type"] = type_name();
  obj["name"] = name();
  obj["position"] = QJsonArray{pos().x(), pos().y()};
  obj["rotation"] = rotation();
  obj["radius"] = rect().width() / kRadiusDivisor;
  const QColor color = brush().color();
  obj["fill_color"] =
    QJsonArray{color.red(), color.green(), color.blue(), color.alpha()};
  return obj;
}

void CircleItem::from_json(const QJsonObject& json) {
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
  if (json.contains("radius")) {
    const qreal radius_value = json["radius"].toDouble();
    if (std::isfinite(radius_value) && radius_value > 0.0 &&
        radius_value >= kMinRadiusPx && radius_value <= kMaxRadiusPx) {
      setRect(QRectF(-radius_value, -radius_value,
                     kRadiusDivisor * radius_value,
                     kRadiusDivisor * radius_value));
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

void CircleItem::paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option,
                       QWidget* widget) {
  // Draw the base ellipse first
  QGraphicsEllipseItem::paint(painter, option, widget);

  // Draw grid if material has Internal grid enabled (radial for circles)
  if (material_model() != nullptr &&
      material_model()->grid_type() == MaterialModel::GridType::Internal) {
    // Calculate extended rect for grid (includes outer ring)
    const QRectF base_rect = rect();
    const qreal radius = base_rect.width() / 2.0;
    const qreal extend = radius * kOuterRingExtendRatio;
    const QRectF extended_rect =
      base_rect.adjusted(-extend, -extend, extend, extend);
    draw_radial_grid(painter, extended_rect, base_rect);
  }
}

void CircleItem::draw_radial_grid(QPainter* painter,
                                  const QRectF& /*extendedRect*/,
                                  const QRectF& baseRect) const {
  auto* material = material_model();
  if (material == nullptr) {
    return;
  }

  painter->save();

  // Remove clipping to allow drawing outside base rect
  painter->setClipping(false);

  // Use composition mode that doesn't darken - draw only lines, no fill
  painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
  painter->setBrush(Qt::NoBrush);
  QPen grid_pen(QColor(0, 0, 0, kGridPenAlpha));
  grid_pen.setWidthF(kGridPenWidth);
  painter->setPen(grid_pen);

  const QPointF center = baseRect.center();
  const qreal radius = baseRect.width() / kRadiusDivisor;
  const double freq_radial = material->grid_frequency_x();
  const double freq_concentric =
    material->grid_frequency_y();  // Used for both inner and outer rings

  // Calculate ring boundaries
  const qreal inner_ring_start_radius = radius * kInnerRingStartRatio;
  const qreal inner_ring_end_radius = radius;
  const qreal outer_ring_start_radius = radius;
  const qreal outer_ring_end_radius = radius * (1.0 + kOuterRingExtendRatio);
  const qreal boundary_margin = radius * kBoundaryRingMargin;

  // Draw concentric circles first (so radial lines appear on top)

  // Draw inner ring concentric circles
  const qreal inner_ring_width =
    inner_ring_end_radius - inner_ring_start_radius;
  const qreal inner_spacing =
    inner_ring_width /
    (freq_concentric + 1);  // +1 accounts for boundary circle
  const qreal first_inner_radius =
    inner_ring_start_radius +
    inner_spacing;  // First circle (closest to center)
  qreal current_inner_radius = first_inner_radius;
  while (current_inner_radius < inner_ring_end_radius - boundary_margin) {
    painter->drawEllipse(center, current_inner_radius, current_inner_radius);
    current_inner_radius += inner_spacing;
  }

  // Draw inner ring boundary circle
  painter->drawEllipse(center, inner_ring_end_radius, inner_ring_end_radius);

  // Draw main boundary circle
  painter->drawEllipse(center, radius, radius);

  // Draw outer ring concentric circles
  const qreal outer_ring_width =
    outer_ring_end_radius - outer_ring_start_radius;
  const qreal outer_spacing =
    outer_ring_width /
    (freq_concentric + 1);  // +1 accounts for boundary circle
  qreal current_outer_radius =
    outer_ring_start_radius + boundary_margin + outer_spacing;
  qreal last_outer_radius =
    outer_ring_start_radius +
    boundary_margin;  // Last circle (farthest from center)
  while (current_outer_radius < outer_ring_end_radius) {
    painter->drawEllipse(center, current_outer_radius, current_outer_radius);
    last_outer_radius = current_outer_radius;
    current_outer_radius += outer_spacing;
  }

  // Draw radial lines (after circles so they appear on top)
  const int num_radial_lines = static_cast<int>(freq_radial);
  // Precompute angles to avoid repeated calculations
  QVector<qreal> angles(num_radial_lines);
  QVector<qreal> cos_values(num_radial_lines);
  QVector<qreal> sin_values(num_radial_lines);
  for (int i = 0; i < num_radial_lines; ++i) {
    angles[i] = (kFullCircleDegrees * i) / num_radial_lines;
    const qreal radians = qDegreesToRadians(angles[i]);
    cos_values[i] = qCos(radians);
    sin_values[i] = qSin(radians);
  }

  for (int i = 0; i < num_radial_lines; ++i) {
    const qreal cos_a = cos_values[i];
    const qreal sin_a = sin_values[i];

    // Inner ring: from first circle to boundary
    const QPointF inner_start =
      center + QPointF(first_inner_radius * cos_a, first_inner_radius * sin_a);
    const QPointF inner_end = center + QPointF(inner_ring_end_radius * cos_a,
                                               inner_ring_end_radius * sin_a);
    painter->drawLine(inner_start, inner_end);

    // Outer ring: from boundary to last circle
    const QPointF outer_start =
      center +
      QPointF(outer_ring_start_radius * cos_a, outer_ring_start_radius * sin_a);
    const QPointF outer_end =
      center + QPointF(last_outer_radius * cos_a, last_outer_radius * sin_a);
    painter->drawLine(outer_start, outer_end);
  }

  painter->restore();
}

QRectF CircleItem::boundingRect() const {
  const QRectF base_rect = QGraphicsEllipseItem::boundingRect();
  // Extend bounding rect to include outer grid ring
  // Use the actual rect() to get the circle's radius
  const qreal radius = rect().width() / kRadiusDivisor;
  const qreal extend = radius * kOuterRingExtendRatio;
  return base_rect.adjusted(-extend, -extend, extend, extend);
}

QVariant CircleItem::itemChange(GraphicsItemChange change,
                                const QVariant& value) {
  if (change == ItemPositionHasChanged || change == ItemRotationHasChanged ||
      change == ItemTransformHasChanged) {
    notify_geometry_changed();
  }
  return QGraphicsEllipseItem::itemChange(change, value);
}
