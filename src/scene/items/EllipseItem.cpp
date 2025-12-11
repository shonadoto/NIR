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
  if (trimmed.isEmpty() || trimmed == name_) {
    return;
  }
  name_ = trimmed;
}

QWidget* EllipseItem::create_properties_widget(QWidget* parent) {
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
    const QString name = json["name"].toString();
    if (!name.isEmpty()) {
      name_ = name;
    }
  }
  if (json.contains("position")) {
    QJsonArray position_array = json["position"].toArray();
    if (position_array.size() >= 2) {
      const double x = position_array[0].toDouble();
      const double y = position_array[1].toDouble();
      if (std::isfinite(x) && std::isfinite(y)) {
        setPos(x, y);
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
      const int r = color_array[0].toInt();
      const int g = color_array[1].toInt();
      const int b = color_array[2].toInt();
      const int a = color_array[3].toInt();
      // Validate color values are in valid range [0, 255]
      if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 &&
          a >= 0 && a <= 255) {
        setBrush(QBrush(QColor(r, g, b, a)));
      }
    }
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
    const QRectF base_rect = rect();
    const qreal max_radius = qMax(base_rect.width(), base_rect.height()) / 2.0;
    const qreal extend = max_radius * kOuterRingExtendRatio;
    const QRectF extended_rect =
      base_rect.adjusted(-extend, -extend, extend, extend);
    draw_radial_grid(painter, extended_rect, base_rect);
  }
}

void EllipseItem::draw_radial_grid(QPainter* painter,
                                   const QRectF& /* extendedRect */,
                                   const QRectF& baseRect) const {
  // material_model_ is already checked in paint() before calling this
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
  const qreal half_width = baseRect.width() / 2.0;
  const qreal half_height = baseRect.height() / 2.0;
  const qreal max_radius = qMax(half_width, half_height);
  const double freq_radial = material_model_->grid_frequency_x();
  const double freq_concentric =
    material_model_->grid_frequency_y();  // Used for both inner and outer rings

  // Calculate ring boundaries (using max_radius as reference)
  const qreal inner_ring_start_radius = max_radius * kInnerRingStartRatio;
  const qreal inner_ring_end_radius = max_radius;
  const qreal outer_ring_start_radius = max_radius;
  const qreal outer_ring_end_radius =
    max_radius * (1.0 + kOuterRingExtendRatio);
  const qreal boundary_margin = max_radius * kBoundaryRingMargin;

  // Helper function to calculate point on ellipse at given scale
  auto ellipse_point = [&](qreal scale, qreal angle) -> QPointF {
    const qreal radians = qDegreesToRadians(angle);
    const qreal cos_a = qCos(radians);
    const qreal sin_a = qSin(radians);
    const qreal ellipse_a = half_width * scale;
    const qreal ellipse_b = half_height * scale;
    const qreal radius =
      (ellipse_a * ellipse_b) / qSqrt(ellipse_b * ellipse_b * cos_a * cos_a +
                                      ellipse_a * ellipse_a * sin_a * sin_a);
    return center + QPointF(radius * cos_a, radius * sin_a);
  };

  // Draw concentric ellipses first (so radial lines appear on top)

  // Draw inner ring concentric ellipses
  const qreal inner_ring_width =
    inner_ring_end_radius - inner_ring_start_radius;
  const qreal inner_spacing =
    inner_ring_width /
    (freq_concentric + 1);  // +1 accounts for boundary ellipse
  const qreal first_inner_radius =
    inner_ring_start_radius +
    inner_spacing;  // First ellipse (closest to center)
  qreal current_inner_radius = first_inner_radius;
  while (current_inner_radius < inner_ring_end_radius - boundary_margin) {
    const qreal scale = current_inner_radius / max_radius;
    const QRectF ellipse_rect(center.x() - half_width * scale,
                              center.y() - half_height * scale,
                              half_width * scale * 2, half_height * scale * 2);
    painter->drawEllipse(ellipse_rect);
    current_inner_radius += inner_spacing;
  }

  // Draw inner ring boundary ellipse
  const qreal inner_boundary_scale = inner_ring_end_radius / max_radius;
  const QRectF inner_boundary_rect(
    center.x() - half_width * inner_boundary_scale,
    center.y() - half_height * inner_boundary_scale,
    half_width * inner_boundary_scale * 2,
    half_height * inner_boundary_scale * 2);
  painter->drawEllipse(inner_boundary_rect);

  // Draw main boundary ellipse
  painter->drawEllipse(baseRect);

  // Draw outer ring concentric ellipses
  const qreal outer_ring_width =
    outer_ring_end_radius - outer_ring_start_radius;
  const qreal outer_spacing =
    outer_ring_width /
    (freq_concentric + 1);  // +1 accounts for boundary ellipse
  qreal current_outer_radius =
    outer_ring_start_radius + boundary_margin + outer_spacing;
  qreal last_outer_radius =
    outer_ring_start_radius +
    boundary_margin;  // Last ellipse (farthest from center)
  while (current_outer_radius < outer_ring_end_radius) {
    const qreal scale = current_outer_radius / max_radius;
    const QRectF ellipse_rect(center.x() - half_width * scale,
                              center.y() - half_height * scale,
                              half_width * scale * 2, half_height * scale * 2);
    painter->drawEllipse(ellipse_rect);
    last_outer_radius = current_outer_radius;
    current_outer_radius += outer_spacing;
  }

  // Draw radial lines (after ellipses so they appear on top)
  const int num_radial_lines = static_cast<int>(freq_radial);
  // Precompute angles to avoid repeated calculations
  QVector<qreal> angles(num_radial_lines);
  for (int i = 0; i < num_radial_lines; ++i) {
    angles[i] = (kFullCircleDegrees * i) / num_radial_lines;
  }

  for (int i = 0; i < num_radial_lines; ++i) {
    const qreal angle = angles[i];

    // Inner ring: from first ellipse to boundary
    const qreal first_inner_scale = first_inner_radius / max_radius;
    const qreal inner_end_scale = inner_ring_end_radius / max_radius;
    const QPointF inner_start = ellipse_point(first_inner_scale, angle);
    const QPointF inner_end = ellipse_point(inner_end_scale, angle);
    painter->drawLine(inner_start, inner_end);

    // Outer ring: from boundary to last ellipse
    const qreal outer_start_scale = outer_ring_start_radius / max_radius;
    const qreal last_outer_scale = last_outer_radius / max_radius;
    const QPointF outer_start = ellipse_point(outer_start_scale, angle);
    const QPointF outer_end = ellipse_point(last_outer_scale, angle);
    painter->drawLine(outer_start, outer_end);
  }

  painter->restore();
}

QRectF EllipseItem::boundingRect() const {
  const QRectF base_rect = QGraphicsEllipseItem::boundingRect();
  // Extend bounding rect to include outer grid ring
  // Use the actual rect() to get the ellipse's dimensions
  const qreal max_radius = qMax(rect().width(), rect().height()) / 2.0;
  const qreal extend = max_radius * kOuterRingExtendRatio;
  return base_rect.adjusted(-extend, -extend, extend, extend);
}

QVariant EllipseItem::itemChange(GraphicsItemChange change,
                                 const QVariant& value) {
  if (change == ItemPositionHasChanged || change == ItemRotationHasChanged ||
      change == ItemTransformHasChanged) {
    notify_geometry_changed();
  }
  return QGraphicsEllipseItem::itemChange(change, value);
}
