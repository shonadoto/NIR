#include "model/MaterialModel.h"

#include "model/core/ModelTypes.h"

MaterialModel::MaterialModel(const Color& color) : color_(color) {}

void MaterialModel::set_color(const Color& color) {
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

void MaterialModel::set_grid_frequency_x(double frequency) {
  if (grid_frequency_x_ == frequency || frequency < 1.0) {
    return;
  }
  grid_frequency_x_ = frequency;
  notify_change(ModelChange{ModelChange::Type::Custom, "grid_frequency_x"});
}

void MaterialModel::set_grid_frequency_y(double frequency) {
  if (grid_frequency_y_ == frequency || frequency < 1.0) {
    return;
  }
  grid_frequency_y_ = frequency;
  notify_change(ModelChange{ModelChange::Type::Custom, "grid_frequency_y"});
}
