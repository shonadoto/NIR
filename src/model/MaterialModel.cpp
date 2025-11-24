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

