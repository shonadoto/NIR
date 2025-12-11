#include "ui/bindings/ShapeModelBinder.h"

#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QLineF>
#include <QPen>
#include <QString>
#include <vector>

#include "scene/ISceneObject.h"
#include "scene/items/CircleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/StickItem.h"
#include "ui/utils/ColorUtils.h"

namespace {
/**
 * @brief Check if an ISceneObject pointer is still valid.
 * @param item Pointer to check (can be nullptr or dangling).
 * @return true if item is valid and still in scene, false otherwise.
 */
auto is_item_valid(const ISceneObject* item) -> bool {
  if (item == nullptr) {
    return false;
  }
  // Check if it's a QGraphicsItem that's still in scene
  if (const auto* graphics_item = dynamic_cast<const QGraphicsItem*>(item)) {
    // If scene() returns nullptr, item was removed from scene or deleted
    return graphics_item->scene() != nullptr;
  }
  // For non-QGraphicsItem ISceneObject implementations, assume valid
  // (though currently all implementations are QGraphicsItem-based)
  return true;
}

auto ColorFromItem(const ISceneObject* item) -> Color {
  if (!is_item_valid(item)) {
    return Color{};
  }
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
  if (!is_item_valid(item)) {
    return;
  }
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
  return Point2D{.x = point.x(), .y = point.y()};
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
  if (item == nullptr || !is_item_valid(item)) {
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

  bindings_[item] = Binding{.model = model, .connection_id = connection_id};
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
  if (item == nullptr || model == nullptr || !is_item_valid(item)) {
    return nullptr;
  }
  if (bindings_.contains(item)) {
    return bindings_[item].model;
  }

  const int connection_id = model->on_changed().connect(
    [this, item](const ModelChange& change) { handle_change(item, change); });

  bindings_[item] = Binding{.model = model, .connection_id = connection_id};
  update_material_binding(item, bindings_[item]);
  item->set_geometry_changed_callback(
    [this, item] { on_item_geometry_changed(item); });
  apply_geometry(item, model);
  ApplyColorToItem(item,
                   model->material() ? model->material()->color() : Color{});
  // Apply material model to scene item so grid settings are displayed
  if (model->material() != nullptr) {
    item->set_material_model(model->material().get());
  }
  return model;
}

auto ShapeModelBinder::model_for(ISceneObject* item) const
  -> std::shared_ptr<ShapeModel> {
  if (item == nullptr || !is_item_valid(item)) {
    return nullptr;
  }
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
      // Validate item is still valid before returning
      if (is_item_valid(entry.first)) {
        return dynamic_cast<QGraphicsItem*>(entry.first);
      }
      // Item is no longer valid, but we still have binding - this is a bug
      // but we'll return nullptr to avoid dangling pointer
    }
  }
  return nullptr;
}

void ShapeModelBinder::unbind_shape(ISceneObject* item) {
  if (item == nullptr) {
    return;
  }
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator == bindings_.end()) {
    return;
  }
  // Only call methods on item if it's still valid
  if (is_item_valid(item)) {
    item->clear_geometry_changed_callback();
  }
  detach_material_binding(binding_iterator->second);
  if (auto model = binding_iterator->second.model) {
    model->on_changed().disconnect(binding_iterator->second.connection_id);
  }
  bindings_.erase(binding_iterator);
}

void ShapeModelBinder::clear_bindings() {
  for (auto& entry : bindings_) {
    // Only call methods on item if it's still valid
    if (is_item_valid(entry.first)) {
      entry.first->clear_geometry_changed_callback();
    }
    if (entry.second.model) {
      entry.second.model->on_changed().disconnect(entry.second.connection_id);
    }
    detach_material_binding(entry.second);
  }
  bindings_.clear();
}

void ShapeModelBinder::cleanup_invalid_bindings() {
  // Collect invalid bindings first to avoid iterator invalidation
  std::vector<ISceneObject*> invalid_items;
  for (const auto& entry : bindings_) {
    if (!is_item_valid(entry.first)) {
      invalid_items.push_back(entry.first);
    }
  }
  // Unbind all invalid items
  for (ISceneObject* item : invalid_items) {
    unbind_shape(item);
  }
}

void ShapeModelBinder::handle_change(ISceneObject* item,
                                     const ModelChange& change) {
  if (item == nullptr) {
    return;
  }
  auto binding_it = bindings_.find(item);
  if (binding_it == bindings_.end()) {
    return;
  }
  auto model = binding_it->second.model;
  if (!model) {
    return;
  }

  // Validate item is still valid - if not, unbind and return
  if (!is_item_valid(item)) {
    unbind_shape(item);
    return;
  }

  switch (change.type) {
    case ModelChange::Type::NameChanged:
      apply_name(item, model->name());
      break;
    case ModelChange::Type::MaterialChanged:
      update_material_binding(item, binding_it->second);
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

void ShapeModelBinder::apply_name(ISceneObject* item, const std::string& name) {
  if (!is_item_valid(item)) {
    return;
  }
  item->set_name(QString::fromStdString(name));
}

void ShapeModelBinder::apply_color(ISceneObject* item, const Color& color) {
  ApplyColorToItem(item, color);
}

auto ShapeModelBinder::extract_color(const ISceneObject* item) -> Color {
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

      // Validate item is still valid - if not, unbind and return
      if (!is_item_valid(item)) {
        unbind_shape(item);
        return;
      }

      apply_color(item, binding_iterator->second.bound_material->color());
    });
}

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
  if (item == nullptr || model == nullptr || !is_item_valid(item)) {
    return;
  }
  auto* graphics_item = dynamic_cast<QGraphicsItem*>(item);
  if (graphics_item == nullptr) {
    return;
  }
  auto binding_it = bindings_.find(item);
  if (binding_it == bindings_.end()) {
    return;
  }

  // Use RAII to ensure flag is reset even if exception occurs
  struct SuppressGuard {
   private:
    bool& flag_;

   public:
    explicit SuppressGuard(bool& flag_ref) : flag_(flag_ref) {
      flag_ = true;
    }
    ~SuppressGuard() {
      flag_ = false;
    }
  };
  const SuppressGuard guard(binding_it->second.suppress_model_geometry_signal);

  model->set_position(ToModelPoint(graphics_item->pos()));
  model->set_rotation_deg(graphics_item->rotation());

  // Check CircleItem first (before QGraphicsEllipseItem) since CircleItem
  // inherits from it
  if (auto* circle_item = dynamic_cast<CircleItem*>(item)) {
    // Circle: rect().width() is diameter, model stores diameter x diameter
    const QRectF rect = circle_item->rect();
    const qreal diameter =
      rect.width();  // For circle, width == height == diameter
    model->set_size(Size2D{.width = diameter, .height = diameter});
  } else if (auto* rect_item =
               dynamic_cast<QGraphicsRectItem*>(graphics_item)) {
    const QRectF rect = rect_item->rect();
    model->set_size(Size2D{.width = rect.width(), .height = rect.height()});
  } else if (auto* ellipse_item =
               dynamic_cast<QGraphicsEllipseItem*>(graphics_item)) {
    // This handles EllipseItem (not CircleItem, as it's checked above)
    const QRectF rect = ellipse_item->rect();
    model->set_size(Size2D{.width = rect.width(), .height = rect.height()});
  } else if (auto* line_item =
               dynamic_cast<QGraphicsLineItem*>(graphics_item)) {
    const qreal length = line_item->line().length();
    model->set_size(
      Size2D{.width = length, .height = line_item->pen().widthF()});
  } else {
    const QSizeF size = graphics_item->boundingRect().size();
    model->set_size(Size2D{.width = size.width(), .height = size.height()});
  }
}

void ShapeModelBinder::apply_geometry(
  ISceneObject* item, const std::shared_ptr<ShapeModel>& model) {
  if (item == nullptr || model == nullptr || !is_item_valid(item)) {
    return;
  }
  auto* graphics_item = dynamic_cast<QGraphicsItem*>(item);
  if (graphics_item == nullptr) {
    return;
  }
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator == bindings_.end()) {
    return;
  }

  // Use RAII to ensure flag is reset even if exception occurs
  // Suppress geometry callback to prevent circular updates when applying
  // geometry from model
  struct SuppressGuard {
   private:
    bool& flag_;

   public:
    explicit SuppressGuard(bool& flag_ref) : flag_(flag_ref) {
      flag_ = true;
    }
    ~SuppressGuard() {
      flag_ = false;
    }
  };
  const SuppressGuard guard(
    binding_iterator->second.suppress_geometry_callback);

  graphics_item->setPos(ToQPoint(model->position()));
  graphics_item->setRotation(model->rotation_deg());

  const Size2D size = model->size();
  constexpr double kDiameterMultiplier = 2.0;
  if (auto* rect_item = dynamic_cast<QGraphicsRectItem*>(graphics_item)) {
    QRectF rect = rect_item->rect();
    rect.setWidth(size.width);
    rect.setHeight(size.height);
    rect_item->setRect(rect);
    rect_item->setTransformOriginPoint(rect_item->boundingRect().center());
  } else if (auto* circle_item = dynamic_cast<CircleItem*>(item)) {
    // Circle: size stores diameter x diameter, rect must be centered at (0,0)
    const qreal radius = size.width / 2.0;
    circle_item->setRect(QRectF(-radius, -radius, kDiameterMultiplier * radius,
                                kDiameterMultiplier * radius));
    // Circle is always centered at (0,0), so transform origin is at (0,0)
    // relative to item
    circle_item->setTransformOriginPoint(QPointF(0, 0));
  } else if (auto* ellipse_item =
               dynamic_cast<QGraphicsEllipseItem*>(graphics_item)) {
    QRectF rect = ellipse_item->rect();
    rect.setWidth(size.width);
    rect.setHeight(size.height);
    ellipse_item->setRect(rect);
    ellipse_item->setTransformOriginPoint(
      ellipse_item->boundingRect().center());
  } else if (auto* line_item =
               dynamic_cast<QGraphicsLineItem*>(graphics_item)) {
    const qreal half_length = size.width / 2.0;
    line_item->setLine(QLineF(-half_length, 0.0, half_length, 0.0));
    line_item->setTransformOriginPoint(line_item->boundingRect().center());
    // For stick items, keep fixed pen width
    constexpr double kStickPenWidth = 2.0;
    if (dynamic_cast<StickItem*>(item) != nullptr) {
      QPen pen = line_item->pen();
      pen.setWidthF(kStickPenWidth);  // Fixed width for stick
      line_item->setPen(pen);
    }
  }
}

void ShapeModelBinder::on_item_geometry_changed(ISceneObject* item) {
  if (item == nullptr) {
    return;
  }
  auto binding_iterator = bindings_.find(item);
  if (binding_iterator == bindings_.end()) {
    return;
  }
  if (binding_iterator->second.suppress_geometry_callback) {
    return;
  }

  // Validate item is still valid - if not, unbind and return
  if (!is_item_valid(item)) {
    unbind_shape(item);
    return;
  }

  update_model_geometry(item, binding_iterator->second.model);
}
