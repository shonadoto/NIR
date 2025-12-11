#include "model/ShapeModel.h"

#include <memory>

#include "model/MaterialModel.h"
#include "model/core/ModelTypes.h"

namespace {
constexpr uint8_t kDefaultColorR = 128;
constexpr uint8_t kDefaultColorG = 128;
constexpr uint8_t kDefaultColorB = 128;
constexpr uint8_t kDefaultColorA = 255;
}  // namespace

ShapeModel::ShapeModel(ShapeType type)
    : type_(type),
      material_(std::make_shared<MaterialModel>(Color{
        kDefaultColorR, kDefaultColorG, kDefaultColorB, kDefaultColorA})) {}

void ShapeModel::assign_material(
  const std::shared_ptr<MaterialModel>& material) {
  material_ = material;
  is_preset_material_ = true;
  notify_change(ModelChange{ModelChange::Type::MaterialChanged, "material"});
}

void ShapeModel::clear_material() {
  // Create new custom material with current color
  const Color current_color = material_ ? material_->color()
                                        : Color{kDefaultColorR, kDefaultColorG,
                                                kDefaultColorB, kDefaultColorA};
  material_ = std::make_shared<MaterialModel>(current_color);
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
  if (!size.is_valid()) {
    return;  // Invalid size, ignore
  }
  size_ = size;
  notify_change(ModelChange{ModelChange::Type::GeometryChanged, "size"});
}

void ShapeModel::set_rotation_deg(double rotation) {
  // Normalize rotation to [0, 360) range
  constexpr double kDegreesInCircle = 360.0;
  double normalized = rotation;
  while (normalized < 0.0) {
    normalized += kDegreesInCircle;
  }
  while (normalized >= kDegreesInCircle) {
    normalized -= kDegreesInCircle;
  }
  if (normalized == rotation_deg_) {
    return;
  }
  rotation_deg_ = normalized;
  notify_change(ModelChange{ModelChange::Type::GeometryChanged, "rotation"});
}
