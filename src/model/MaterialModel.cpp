#include "model/MaterialModel.h"

MaterialModel::MaterialModel(const Color &color)
    : color_(color) {}

void MaterialModel::set_color(const Color &color) {
    if (color == color_) {
        return;
    }
    color_ = color;
    notify_change(ModelChange{ModelChange::Type::ColorChanged, "color"});
}

void MaterialModel::set_grid_type(GridType type) {
    if (grid_type_ == type) {
        return;
    }
    grid_type_ = type;
    notify_change(ModelChange{ModelChange::Type::Custom, "grid_type"});
}

void MaterialModel::set_grid_frequency(double frequency) {
    if (grid_frequency_ == frequency || frequency <= 0.0) {
        return;
    }
    grid_frequency_ = frequency;
    notify_change(ModelChange{ModelChange::Type::Custom, "grid_frequency"});
}

