#pragma once

#include "model/ShapeModel.h"
#include "model/core/ModelTypes.h"

namespace ShapeConstants {
constexpr double kStickThickness = 2.0;
constexpr double kMinCircleSize = 80.0;
constexpr double kMinRectangleWidth = 100.0;
constexpr double kMinRectangleHeight = 60.0;
}  // namespace ShapeConstants

class ShapeSizeConverter {
 public:
  // Convert size between different shape types
  static auto convert(ShapeModel::ShapeType from, ShapeModel::ShapeType to_type,
                      const Size2D& size) -> Size2D;

  // Ensure minimum size for a given shape type
  static auto ensure_minimum(const Size2D& size,
                             ShapeModel::ShapeType type) -> Size2D;
};
