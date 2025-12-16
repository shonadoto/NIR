#include "commands/ShapeCommands.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <algorithm>
#include <string>
#include <variant>

#include "model/DocumentModel.h"
#include "model/ShapeModel.h"
#include "model/ShapeSizeConverter.h"
#include "scene/ISceneObject.h"
#include "ui/bindings/ShapeModelBinder.h"
#include "ui/controller/DocumentController.h"
#include "ui/editor/EditorArea.h"

// CreateShapeCommand
CreateShapeCommand::CreateShapeCommand(DocumentModel* document,
                                       ShapeModelBinder* binder,
                                       EditorArea* editor_area,
                                       ShapeModel::ShapeType type,
                                       std::string name)
    : document_(document),
      binder_(binder),
      editor_area_(editor_area),
      type_(type),
      name_(std::move(name)) {}

auto CreateShapeCommand::execute() -> bool {
  if (document_ == nullptr || binder_ == nullptr || editor_area_ == nullptr) {
    return false;
  }

  // Create shape in document
  created_shape_ = document_->create_shape(type_, name_);
  if (created_shape_ == nullptr) {
    return false;
  }

  // Get substrate center for positioning
  const QPointF center = editor_area_->substrate_center();

  // Create scene item
  created_item_ = DocumentController::create_item_for_shape(created_shape_);
  if (created_item_ == nullptr) {
    document_->remove_shape(created_shape_);
    created_shape_.reset();
    return false;
  }

  // Add to scene
  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    delete created_item_;
    document_->remove_shape(created_shape_);
    created_shape_.reset();
    return false;
  }

  auto* graphics_item = dynamic_cast<QGraphicsItem*>(created_item_);
  if (graphics_item == nullptr) {
    delete created_item_;
    document_->remove_shape(created_shape_);
    created_shape_.reset();
    return false;
  }

  // Set position to substrate center
  // For items with center-based positioning (like CircleItem), we need to
  // adjust
  const QRectF item_rect = graphics_item->boundingRect();
  const QPointF item_center = item_rect.center();
  const QPointF position = center - item_center;

  graphics_item->setPos(position);
  created_shape_->set_position(Point2D{.x = position.x(), .y = position.y()});

  scene->addItem(graphics_item);
  binder_->attach_shape(created_item_, created_shape_);

  return true;
}

auto CreateShapeCommand::undo() -> bool {
  if (created_shape_ == nullptr || created_item_ == nullptr) {
    return false;
  }

  // Unbind first
  if (binder_ != nullptr) {
    binder_->unbind_shape(created_item_);
  }

  // Remove from scene
  if (editor_area_ != nullptr) {
    auto* scene = editor_area_->scene();
    if (scene != nullptr) {
      auto* graphics_item = dynamic_cast<QGraphicsItem*>(created_item_);
      if (graphics_item != nullptr) {
        scene->removeItem(graphics_item);
        delete graphics_item;
      }
    }
  }

  // Remove from document
  if (document_ != nullptr) {
    document_->remove_shape(created_shape_);
  }

  created_item_ = nullptr;
  return true;
}

auto CreateShapeCommand::description() const -> std::string {
  return "Create " + (name_.empty() ? "Shape" : name_);
}

// DeleteShapeCommand
DeleteShapeCommand::DeleteShapeCommand(DocumentModel* document,
                                       ShapeModelBinder* binder,
                                       EditorArea* editor_area,
                                       const std::shared_ptr<ShapeModel>& shape)
    : document_(document),
      binder_(binder),
      editor_area_(editor_area),
      shape_(shape) {
  if (shape_ != nullptr) {
    saved_position_ = shape_->position();
    saved_size_ = shape_->size();
    saved_rotation_ = shape_->rotation_deg();
    saved_name_ = shape_->name();
    saved_type_ = shape_->type();
  }
}

auto DeleteShapeCommand::execute() -> bool {
  if (document_ == nullptr || binder_ == nullptr || editor_area_ == nullptr ||
      shape_ == nullptr) {
    return false;
  }

  // Find item for shape
  item_ = dynamic_cast<ISceneObject*>(binder_->item_for(shape_));
  if (item_ == nullptr) {
    return false;
  }

  // Find index in document
  const auto& shapes = document_->shapes();
  const auto shape_it = std::find(shapes.begin(), shapes.end(), shape_);
  if (shape_it == shapes.end()) {
    return false;
  }
  shape_index_ = static_cast<int>(std::distance(shapes.begin(), shape_it));

  // Unbind
  binder_->unbind_shape(item_);

  // Remove from scene
  auto* scene = editor_area_->scene();
  if (scene != nullptr) {
    auto* graphics_item = dynamic_cast<QGraphicsItem*>(item_);
    if (graphics_item != nullptr) {
      scene->removeItem(graphics_item);
      delete graphics_item;
    }
  }

  // Remove from document
  document_->remove_shape(shape_);

  return true;
}

auto DeleteShapeCommand::undo() -> bool {
  if (document_ == nullptr || binder_ == nullptr || editor_area_ == nullptr ||
      shape_ == nullptr) {
    return false;
  }

  // Restore shape properties
  shape_->set_position(saved_position_);
  shape_->set_size(saved_size_);
  shape_->set_rotation_deg(saved_rotation_);
  shape_->set_name(saved_name_);
  shape_->set_type(saved_type_);

  // Re-add shape to document
  // Note: We need to manually add it back since DocumentModel doesn't have
  // insert_at. We'll add it to the end, which is acceptable as order doesn't
  // matter for shapes. But actually, we can't directly add to shapes_ vector
  // as it's private. So we need to use a workaround - create a new shape and
  // copy properties, or add a method to DocumentModel.
  // For now, let's create a new shape and copy all properties
  auto restored_shape = document_->create_shape(saved_type_, saved_name_);
  if (restored_shape == nullptr) {
    return false;
  }
  restored_shape->set_position(saved_position_);
  restored_shape->set_size(saved_size_);
  restored_shape->set_rotation_deg(saved_rotation_);

  // Copy material if shape had one
  if (shape_->material() != nullptr) {
    restored_shape->assign_material(shape_->material());
  }

  // Recreate scene item
  item_ = DocumentController::create_item_for_shape(restored_shape);
  if (item_ == nullptr) {
    document_->remove_shape(restored_shape);
    return false;
  }

  // Add to scene
  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    delete item_;
    document_->remove_shape(restored_shape);
    return false;
  }

  auto* graphics_item = dynamic_cast<QGraphicsItem*>(item_);
  if (graphics_item == nullptr) {
    delete item_;
    document_->remove_shape(restored_shape);
    return false;
  }

  graphics_item->setPos(saved_position_.x, saved_position_.y);
  graphics_item->setRotation(saved_rotation_);
  scene->addItem(graphics_item);
  binder_->attach_shape(item_, restored_shape);

  // Update shape_ to point to restored shape
  shape_ = restored_shape;
  return true;
}

auto DeleteShapeCommand::description() const -> std::string {
  return "Delete " + (saved_name_.empty() ? "Shape" : saved_name_);
}

// ModifyShapePropertyCommand
ModifyShapePropertyCommand::ModifyShapePropertyCommand(
  const std::shared_ptr<ShapeModel>& shape, Property property,
  const std::variant<std::string, Point2D, Size2D, double, Color,
                     std::shared_ptr<MaterialModel>>& new_value)
    : shape_(shape), property_(property), new_value_(new_value) {
  // Save old value
  if (shape_ != nullptr) {
    switch (property) {
      case Property::kName:
        old_value_ = shape_->name();
        break;
      case Property::kPosition:
        old_value_ = shape_->position();
        break;
      case Property::kSize:
        old_value_ = shape_->size();
        break;
      case Property::kRotation:
        old_value_ = shape_->rotation_deg();
        break;
      case Property::kColor:
        old_value_ = shape_->custom_color();
        break;
      case Property::kMaterial:
        old_value_ = shape_->material();
        break;
    }
  }
}

auto ModifyShapePropertyCommand::execute() -> bool {
  if (shape_ == nullptr) {
    return false;
  }

  switch (property_) {
    case Property::kName:
      if (std::holds_alternative<std::string>(new_value_)) {
        shape_->set_name(std::get<std::string>(new_value_));
      }
      break;
    case Property::kPosition:
      if (std::holds_alternative<Point2D>(new_value_)) {
        shape_->set_position(std::get<Point2D>(new_value_));
      }
      break;
    case Property::kSize:
      if (std::holds_alternative<Size2D>(new_value_)) {
        shape_->set_size(std::get<Size2D>(new_value_));
      }
      break;
    case Property::kRotation:
      if (std::holds_alternative<double>(new_value_)) {
        shape_->set_rotation_deg(std::get<double>(new_value_));
      }
      break;
    case Property::kColor:
      if (std::holds_alternative<Color>(new_value_)) {
        shape_->set_custom_color(std::get<Color>(new_value_));
      }
      break;
    case Property::kMaterial:
      if (std::holds_alternative<std::shared_ptr<MaterialModel>>(new_value_)) {
        auto material = std::get<std::shared_ptr<MaterialModel>>(new_value_);
        if (material != nullptr) {
          shape_->assign_material(material);
        } else {
          shape_->clear_material();
        }
      }
      break;
  }

  return true;
}

auto ModifyShapePropertyCommand::undo() -> bool {
  if (shape_ == nullptr) {
    return false;
  }

  switch (property_) {
    case Property::kName:
      if (std::holds_alternative<std::string>(old_value_)) {
        shape_->set_name(std::get<std::string>(old_value_));
      }
      break;
    case Property::kPosition:
      if (std::holds_alternative<Point2D>(old_value_)) {
        shape_->set_position(std::get<Point2D>(old_value_));
      }
      break;
    case Property::kSize:
      if (std::holds_alternative<Size2D>(old_value_)) {
        shape_->set_size(std::get<Size2D>(old_value_));
      }
      break;
    case Property::kRotation:
      if (std::holds_alternative<double>(old_value_)) {
        shape_->set_rotation_deg(std::get<double>(old_value_));
      }
      break;
    case Property::kColor:
      if (std::holds_alternative<Color>(old_value_)) {
        shape_->set_custom_color(std::get<Color>(old_value_));
      }
      break;
    case Property::kMaterial:
      if (std::holds_alternative<std::shared_ptr<MaterialModel>>(old_value_)) {
        auto material = std::get<std::shared_ptr<MaterialModel>>(old_value_);
        if (material != nullptr) {
          shape_->assign_material(material);
        } else {
          shape_->clear_material();
        }
      }
      break;
  }

  return true;
}

auto ModifyShapePropertyCommand::description() const -> std::string {
  switch (property_) {
    case Property::kName:
      return "Rename Shape";
    case Property::kPosition:
      return "Move Shape";
    case Property::kSize:
      return "Resize Shape";
    case Property::kRotation:
      return "Rotate Shape";
    case Property::kColor:
      return "Change Shape Color";
    case Property::kMaterial:
      return "Change Shape Material";
  }
  return "Modify Shape";
}

auto ModifyShapePropertyCommand::merge_with(const Command& other) -> bool {
  const auto* other_cmd =
    dynamic_cast<const ModifyShapePropertyCommand*>(&other);
  if (other_cmd == nullptr) {
    return false;
  }

  // Only merge if same shape and same property
  if (shape_ != other_cmd->shape_ || property_ != other_cmd->property_) {
    return false;
  }

  // Only merge certain properties (e.g., position, rotation for dragging)
  if (property_ == Property::kPosition || property_ == Property::kRotation) {
    // Update new_value_ to other's new_value_ (we're merging forward)
    new_value_ = other_cmd->new_value_;
    return true;
  }

  return false;
}

// ChangeShapeTypeCommand
ChangeShapeTypeCommand::ChangeShapeTypeCommand(
  DocumentModel* document, ShapeModelBinder* binder, EditorArea* editor_area,
  std::shared_ptr<ShapeModel> shape, ShapeModel::ShapeType new_type)
    : document_(document),
      binder_(binder),
      editor_area_(editor_area),
      shape_(std::move(shape)),
      new_type_(new_type) {
  if (shape_ != nullptr) {
    old_type_ = shape_->type();
  }
}

auto ChangeShapeTypeCommand::execute() -> bool {
  if (document_ == nullptr || binder_ == nullptr || editor_area_ == nullptr ||
      shape_ == nullptr) {
    return false;
  }

  if (shape_->type() == new_type_) {
    return false;  // No change needed
  }

  // Find old item
  old_item_ = dynamic_cast<ISceneObject*>(binder_->item_for(shape_));
  if (old_item_ == nullptr) {
    return false;
  }

  // Get current properties
  const auto old_size = shape_->size();
  // const auto old_position = shape_->position();
  const auto old_rotation = shape_->rotation_deg();
  const auto old_name = shape_->name();

  // Convert size
  const auto new_size =
    DocumentController::convert_shape_size(old_type_, new_type_, old_size);

  // Update model
  shape_->set_type(new_type_);
  shape_->set_size(new_size);

  // Use DocumentController to replace the item
  DocumentController controller;
  controller.set_document_model(document_);
  controller.set_shape_binder(binder_);
  controller.set_editor_area(editor_area_);

  auto* old_graphics_item = dynamic_cast<QGraphicsItem*>(old_item_);
  if (old_graphics_item == nullptr) {
    return false;
  }

  const QPointF center = old_graphics_item->sceneBoundingRect().center();
  controller.replace_shape_item(old_item_, shape_, center, old_rotation,
                                QString::fromStdString(old_name));

  // Find new item
  new_item_ = dynamic_cast<ISceneObject*>(binder_->item_for(shape_));

  return new_item_ != nullptr;
}

auto ChangeShapeTypeCommand::undo() -> bool {
  if (document_ == nullptr || binder_ == nullptr || editor_area_ == nullptr ||
      shape_ == nullptr) {
    return false;
  }

  // Get current properties
  const auto current_size = shape_->size();
  // const auto current_position = shape_->position();
  const auto current_rotation = shape_->rotation_deg();
  const auto current_name = shape_->name();

  // Convert size back
  const auto restored_size =
    DocumentController::convert_shape_size(new_type_, old_type_, current_size);

  // Update model
  shape_->set_type(old_type_);
  shape_->set_size(restored_size);

  // Use DocumentController to replace the item back
  DocumentController controller;
  controller.set_document_model(document_);
  controller.set_shape_binder(binder_);
  controller.set_editor_area(editor_area_);

  if (new_item_ != nullptr) {
    auto* new_graphics_item = dynamic_cast<QGraphicsItem*>(new_item_);
    if (new_graphics_item != nullptr) {
      const QPointF center = new_graphics_item->sceneBoundingRect().center();
      controller.replace_shape_item(new_item_, shape_, center, current_rotation,
                                    QString::fromStdString(current_name));
    }
  }

  // Find old item (should be restored)
  old_item_ = dynamic_cast<ISceneObject*>(binder_->item_for(shape_));

  return old_item_ != nullptr;
}

auto ChangeShapeTypeCommand::description() const -> std::string {
  return "Change Shape Type";
}
