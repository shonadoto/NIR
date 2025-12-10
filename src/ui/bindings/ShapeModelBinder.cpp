#include "ui/bindings/ShapeModelBinder.h"

#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QLineF>
#include <QPen>
#include <QString>

#include "scene/ISceneObject.h"
#include "scene/items/CircleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/StickItem.h"
#include "ui/utils/ColorUtils.h"

namespace {
auto ColorFromItem(const ISceneObject* item) -> Color {
  if (const auto* graphics_item = dynamic_cast<const QGraphicsItem*>(item)) {
    if (const auto* rect =
          dynamic_cast<const QGraphicsRectItem*>(graphics_item)) {
      return to_model_color(rect->brush().color());
    }
    if (const auto* ellipse =
          dynamic_cast<const QGraphicsEllipseItem*>(graphics_item)) {
      return to_model_color(ellipse->brush().color());
    }
    if (const auto* line =
          dynamic_cast<const QGraphicsLineItem*>(graphics_item)) {
      return to_model_color(line->pen().color());
    }
  }
  return Color{};
}

void ApplyColorToItem(ISceneObject* item, const Color& color) {
  const QColor qcolor = to_qcolor(color);
  if (auto* graphics_item = dynamic_cast<QGraphicsItem*>(item)) {
    if (auto* rect = dynamic_cast<QGraphicsRectItem*>(graphics_item)) {
      QBrush brush = rect->brush();
      brush.setColor(qcolor);
      rect->setBrush(brush);
    } else if (auto* ellipse =
                 dynamic_cast<QGraphicsEllipseItem*>(graphics_item)) {
      QBrush brush = ellipse->brush();
      brush.setColor(qcolor);
      ellipse->setBrush(brush);
    } else if (auto* line = dynamic_cast<QGraphicsLineItem*>(graphics_item)) {
      QPen pen = line->pen();
      pen.setColor(qcolor);
      line->setPen(pen);
      // QGraphicsLineItem doesn't have setBrush, but ensure no fill is applied
      // StickItem handles this in its paint method
    }
  }
}

auto ToModelPoint(const QPointF& point) -> Point2D {
  return {point.x(), point.y()};
}

auto ToQPoint(const Point2D& point) -> QPointF {
  return {point.x, point.y};
}

}  // namespace

ShapeModelBinder::ShapeModelBinder(DocumentModel& document)
    : document_(document) {}

namespace {
ShapeModel::ShapeType type_from_item(ISceneObject* item) {
  if (dynamic_cast<RectangleItem*>(item) != nullptr) {
    return ShapeModel::ShapeType::Rectangle;
  }
  if (dynamic_cast<EllipseItem*>(item) != nullptr) {
    return ShapeModel::ShapeType::Ellipse;
  }
  if (dynamic_cast<CircleItem*>(item) != nullptr) {
    return ShapeModel::ShapeType::Circle;
  }
  if (dynamic_cast<StickItem*>(item) != nullptr) {
    return ShapeModel::ShapeType::Stick;
  }
  return ShapeModel::ShapeType::Rectangle;
}
}  // namespace

auto ShapeModelBinder::bind_shape(ISceneObject* item)
  -> std::shared_ptr<ShapeModel> {
  if (item == nullptr) {
    return nullptr;
  }
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator != bindings_.end()) {
    return binding_iterator->second.model;
  }

  auto model =
    document_.create_shape(type_from_item(item), item->name().toStdString());
  if (model->material()) {
    model->material()->set_color(ColorFromItem(item));
  }

  const int connection_id = model->on_changed().connect(
    [this, item](const ModelChange& change) { handle_change(item, change); });

  bindings_[item] = Binding{model, connection_id};
  update_material_binding(item, bindings_[item]);
  update_model_geometry(item, model);
  item->set_geometry_changed_callback(
    [this, item] { on_item_geometry_changed(item); });
  ApplyColorToItem(item,
                   model->material() ? model->material()->color() : Color{});
  return model;
}

auto ShapeModelBinder::attach_shape(ISceneObject* item,
                                    const std::shared_ptr<ShapeModel>& model)
  -> std::shared_ptr<ShapeModel> {
  if (item == nullptr || model == nullptr) {
    return nullptr;
  }
  if (bindings_.contains(item)) {
    return bindings_[item].model;
  }

  const int connection_id = model->on_changed().connect(
    [this, item](const ModelChange& change) { handle_change(item, change); });

  bindings_[item] = Binding{model, connection_id};
  update_material_binding(item, bindings_[item]);
  item->set_geometry_changed_callback(
    [this, item] { on_item_geometry_changed(item); });
  apply_geometry(item, model);
  ApplyColorToItem(item,
                   model->material() ? model->material()->color() : Color{});
  return model;
}

auto ShapeModelBinder::model_for(ISceneObject* item) const
  -> std::shared_ptr<ShapeModel> {
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator != bindings_.end()) {
    return binding_iterator->second.model;
  }
  return nullptr;
}

auto ShapeModelBinder::item_for(const std::shared_ptr<ShapeModel>& model) const
  -> QGraphicsItem* {
  for (const auto& entry : bindings_) {
    if (entry.second.model == model) {
      return dynamic_cast<QGraphicsItem*>(entry.first);
    }
  }
  return nullptr;
}

void ShapeModelBinder::unbind_shape(ISceneObject* item) {
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator == bindings_.end()) {
    return;
  }
  item->clear_geometry_changed_callback();
  detach_material_binding(binding_iterator->second);
  if (auto model = binding_iterator->second.model) {
    model->on_changed().disconnect(binding_iterator->second.connection_id);
  }
  bindings_.erase(binding_iterator);
}

void ShapeModelBinder::clear_bindings() {
  for (auto& entry : bindings_) {
    entry.first->clear_geometry_changed_callback();
    if (entry.second.model) {
      entry.second.model->on_changed().disconnect(entry.second.connection_id);
    }
    detach_material_binding(entry.second);
  }
  bindings_.clear();
}

void ShapeModelBinder::handle_change(ISceneObject* item,
                                     const ModelChange& change) {
  auto bindingIt = bindings_.find(item);
  if (bindingIt == bindings_.end()) {
    return;
  }
  auto model = bindingIt->second.model;
  if (!model) {
    return;
  }

  switch (change.type) {
    case ModelChange::Type::NameChanged:
      apply_name(item, model->name());
      break;
    case ModelChange::Type::MaterialChanged:
      update_material_binding(item, bindingIt->second);
      [[fallthrough]];
    case ModelChange::Type::ColorChanged: {
      const Color color =
        model->material() ? model->material()->color() : Color{};
      ApplyColorToItem(item, color);
      break;
    }
    case ModelChange::Type::GeometryChanged:
      apply_geometry(item, model);
      break;
    default:
      break;
  }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ShapeModelBinder::apply_name(ISceneObject* item, const std::string& name) {
  item->set_name(QString::fromStdString(name));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ShapeModelBinder::apply_color(ISceneObject* item, const Color& color) {
  ApplyColorToItem(item, color);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto ShapeModelBinder::extract_color(const ISceneObject* item) const -> Color {
  return ColorFromItem(item);
}

void ShapeModelBinder::update_material_binding(ISceneObject* item,
                                               Binding& binding) {
  detach_material_binding(binding);
  if (!binding.model) {
    return;
  }
  auto material = binding.model->material();
  if (!material) {
    return;
  }
  binding.bound_material = material;
  MaterialModel* material_ptr = material.get();
  binding.material_connection_id = material->on_changed().connect(
    [this, item, material_ptr](const ModelChange& change) {
      if (change.type != ModelChange::Type::ColorChanged) {
        return;
      }
      auto binding_iterator = bindings_.find(item);
      if (binding_iterator == bindings_.end()) {
        return;
      }
      if (binding_iterator->second.bound_material == nullptr ||
          binding_iterator->second.bound_material.get() != material_ptr) {
        return;
      }
      apply_color(item, binding_iterator->second.bound_material->color());
    });
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ShapeModelBinder::detach_material_binding(Binding& binding) {
  if (binding.bound_material && binding.material_connection_id != 0) {
    binding.bound_material->on_changed().disconnect(
      binding.material_connection_id);
  }
  binding.material_connection_id = 0;
  binding.bound_material.reset();
}

void ShapeModelBinder::update_model_geometry(
  ISceneObject* item, const std::shared_ptr<ShapeModel>& model) {
  if (item == nullptr || model == nullptr) {
    return;
  }
  auto* graphicsItem = dynamic_cast<QGraphicsItem*>(item);
  if (graphicsItem == nullptr) {
    return;
  }
  auto bindingIt = bindings_.find(item);
  if (bindingIt != bindings_.end()) {
    bindingIt->second.suppress_model_geometry_signal = true;
  }
  model->set_position(ToModelPoint(graphicsItem->pos()));
  model->set_rotation_deg(graphicsItem->rotation());

  // Check CircleItem first (before QGraphicsEllipseItem) since CircleItem
  // inherits from it
  if (auto* circleItem = dynamic_cast<CircleItem*>(item)) {
    // Circle: rect().width() is diameter, model stores diameter x diameter
    const QRectF rect = circleItem->rect();
    const qreal diameter =
      rect.width();  // For circle, width == height == diameter
    model->set_size(Size2D{diameter, diameter});
  } else if (auto* rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
    const QRectF rect = rectItem->rect();
    model->set_size(Size2D{rect.width(), rect.height()});
  } else if (auto* ellipseItem =
               dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
    // This handles EllipseItem (not CircleItem, as it's checked above)
    const QRectF rect = ellipseItem->rect();
    model->set_size(Size2D{rect.width(), rect.height()});
  } else if (auto* lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
    const qreal length = lineItem->line().length();
    model->set_size(Size2D{length, lineItem->pen().widthF()});
  } else {
    const QSizeF size = graphicsItem->boundingRect().size();
    model->set_size(Size2D{size.width(), size.height()});
  }

  if (bindingIt != bindings_.end()) {
    bindingIt->second.suppress_model_geometry_signal = false;
  }
}

void ShapeModelBinder::apply_geometry(
  ISceneObject* item, const std::shared_ptr<ShapeModel>& model) {
  if (item == nullptr || model == nullptr) {
    return;
  }
  auto* graphicsItem = dynamic_cast<QGraphicsItem*>(item);
  if (graphicsItem == nullptr) {
    return;
  }
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator != bindings_.end()) {
    if (binding_iterator->second.suppress_model_geometry_signal) {
      return;
    }
    binding_iterator->second.suppress_geometry_callback = true;
  }

  graphicsItem->setPos(ToQPoint(model->position()));
  graphicsItem->setRotation(model->rotation_deg());

  const Size2D size = model->size();
  constexpr double kDiameterMultiplier = 2.0;
  if (auto* rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
    QRectF rect = rectItem->rect();
    rect.setWidth(size.width);
    rect.setHeight(size.height);
    rectItem->setRect(rect);
    rectItem->setTransformOriginPoint(rectItem->boundingRect().center());
  } else if (auto* circleItem = dynamic_cast<CircleItem*>(item)) {
    // Circle: size stores diameter x diameter, rect must be centered at (0,0)
    const qreal radius = size.width / 2.0;
    circleItem->setRect(QRectF(-radius, -radius, kDiameterMultiplier * radius,
                               kDiameterMultiplier * radius));
    // Circle is always centered at (0,0), so transform origin is at (0,0)
    // relative to item
    circleItem->setTransformOriginPoint(QPointF(0, 0));
  } else if (auto* ellipseItem =
               dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
    QRectF rect = ellipseItem->rect();
    rect.setWidth(size.width);
    rect.setHeight(size.height);
    ellipseItem->setRect(rect);
    ellipseItem->setTransformOriginPoint(ellipseItem->boundingRect().center());
  } else if (auto* lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
    const qreal halfLength = size.width / 2.0;
    lineItem->setLine(QLineF(-halfLength, 0.0, halfLength, 0.0));
    lineItem->setTransformOriginPoint(lineItem->boundingRect().center());
    // For stick items, keep fixed pen width
    constexpr double kStickPenWidth = 2.0;
    if (dynamic_cast<StickItem*>(item) != nullptr) {
      QPen pen = lineItem->pen();
      pen.setWidthF(kStickPenWidth);  // Fixed width for stick
      lineItem->setPen(pen);
    }
  }

  if (binding_iterator != bindings_.end()) {
    binding_iterator->second.suppress_geometry_callback = false;
  }
}

void ShapeModelBinder::on_item_geometry_changed(ISceneObject* item) {
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator == bindings_.end()) {
    return;
  }
  if (binding_iterator->second.suppress_geometry_callback) {
    return;
  }
  update_model_geometry(item, binding_iterator->second.model);
}
