#include "model/SubstrateModel.h"

#include "model/core/ModelTypes.h"

SubstrateModel::SubstrateModel(const Size2D& size, const Color& color)
    : size_(size), color_(color) {}

void SubstrateModel::set_size(const Size2D& size) {
  if (size.width == size_.width && size.height == size_.height) {
    return;
  }
  size_ = size;
  notify_change(ModelChange{ModelChange::Type::SizeChanged, "size"});
}

void SubstrateModel::set_color(const Color& color) {
  if (color == color_) {
    return;
  }
  color_ = color;
  notify_change(ModelChange{ModelChange::Type::ColorChanged, "color"});
}
