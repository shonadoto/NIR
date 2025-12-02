#include "model/ShapeModel.h"

ShapeModel::ShapeModel(ShapeType type)
    : type_(type)
    , custom_color_{128, 128, 128, 255} {}

void ShapeModel::assign_material(const std::shared_ptr<MaterialModel> &material) {
    material_ = material;
    notify_change(ModelChange{ModelChange::Type::MaterialChanged, "material"});
}

void ShapeModel::clear_material() {
    if (!material_) {
        return;
    }
    material_.reset();
    notify_change(ModelChange{ModelChange::Type::MaterialChanged, "material"});
}

void ShapeModel::set_custom_color(const Color &color) {
    if (color == custom_color_) {
        return;
    }
    custom_color_ = color;
    if (!material_) {
        notify_change(ModelChange{ModelChange::Type::ColorChanged, "custom_color"});
    }
}

void ShapeModel::set_type(ShapeType type) {
    if (type_ == type) {
        return;
    }
    type_ = type;
    notify_change(ModelChange{ModelChange::Type::Custom, "type"});
}

void ShapeModel::set_position(const Point2D &pos) {
    if (pos == position_) {
        return;
    }
    position_ = pos;
    notify_change(ModelChange{ModelChange::Type::GeometryChanged, "position"});
}

void ShapeModel::set_size(const Size2D &size) {
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

