#include "model/ShapeModel.h"

ShapeModel::ShapeModel(ShapeType type)
    : type_(type),
      material_(std::make_shared<MaterialModel>(Color{128, 128, 128, 255})),
      is_preset_material_(false) {}

void ShapeModel::assign_material(
  const std::shared_ptr<MaterialModel>& material) {
  material_ = material;
  is_preset_material_ = true;
  notify_change(ModelChange{ModelChange::Type::MaterialChanged, "material"});
}

void ShapeModel::clear_material() {
  // Create new custom material with current color
  Color currentColor =
    material_ ? material_->color() : Color{128, 128, 128, 255};
  material_ = std::make_shared<MaterialModel>(currentColor);
  is_preset_material_ = false;
  notify_change(ModelChange{ModelChange::Type::MaterialChanged, "material"});
}

void ShapeModel::set_custom_color(const Color& color) {
  if (!material_) {
    material_ = std::make_shared<MaterialModel>(color);
    is_preset_material_ = false;
  } else if (!is_preset_material_) {
    material_->set_color(color);
  }
  // If preset material, don't change color
}

void ShapeModel::set_type(ShapeType type) {
  if (type_ == type) {
    return;
  }
  type_ = type;
  notify_change(ModelChange{ModelChange::Type::Custom, "type"});
}

void ShapeModel::set_position(const Point2D& pos) {
  if (pos == position_) {
    return;
  }
  position_ = pos;
  notify_change(ModelChange{ModelChange::Type::GeometryChanged, "position"});
}

void ShapeModel::set_size(const Size2D& size) {
  if (size == size_) {
    return;
  }
  size_ = size;
  notify_change(ModelChange{ModelChange::Type::GeometryChanged, "size"});
}

void ShapeModel::set_rotation_deg(double rotation) {
  if (rotation == rotation_deg_) {
    return;
  }
  rotation_deg_ = rotation;
  notify_change(ModelChange{ModelChange::Type::GeometryChanged, "rotation"});
}
