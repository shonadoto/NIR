#include "model/ShapeSizeConverter.h"

#include <algorithm>

#include "model/core/ModelTypes.h"

namespace {
constexpr double kMinSizeThreshold = 1.0;
}  // namespace

auto ShapeSizeConverter::convert(ShapeModel::ShapeType from,
                                 ShapeModel::ShapeType to_type,
                                 const Size2D& size) -> Size2D {
  if (from == to_type) {
    return size;
  }

  switch (from) {
    case ShapeModel::ShapeType::Circle: {
      // Circle: size stores diameter x diameter
      const double diameter =
        size.width;  // Both width and height are the same (diameter)
      switch (to_type) {
        case ShapeModel::ShapeType::Ellipse:
        case ShapeModel::ShapeType::Rectangle:
          // Use diameter as both width and height (square shape)
          return Size2D{diameter, diameter};
        case ShapeModel::ShapeType::Stick:
          return Size2D{diameter, ShapeConstants::kStickThickness};
        default:
          return size;
      }
    }
    case ShapeModel::ShapeType::Ellipse:
    case ShapeModel::ShapeType::Rectangle: {
      // Ellipse/Rectangle: size stores width x height
      switch (to_type) {
        case ShapeModel::ShapeType::Circle: {
          // Use max dimension as diameter
          const double diameter = std::max(size.width, size.height);
          return Size2D{diameter, diameter};
        }
        case ShapeModel::ShapeType::Ellipse:
        case ShapeModel::ShapeType::Rectangle:
          // Keep same dimensions
          return size;
        case ShapeModel::ShapeType::Stick: {
          const double length = std::max(size.width, size.height);
          return Size2D{length, ShapeConstants::kStickThickness};
        }
        default:
          return size;
      }
    }
    case ShapeModel::ShapeType::Stick: {
      // Stick: size stores length x thickness (thickness is always
      // kStickThickness)
      const double length = size.width;
      switch (to_type) {
        case ShapeModel::ShapeType::Circle: {
          // Use length as diameter
          return Size2D{length, length};
        }
        case ShapeModel::ShapeType::Ellipse:
        case ShapeModel::ShapeType::Rectangle:
          // Use length as both width and height (square shape)
          return Size2D{length, length};
        default:
          return size;
      }
    }
    default:
      return size;
  }
}

auto ShapeSizeConverter::ensure_minimum(const Size2D& size,
                                        ShapeModel::ShapeType type) -> Size2D {
  Size2D result = size;

  switch (type) {
    case ShapeModel::ShapeType::Circle: {
      if (result.width < kMinSizeThreshold) {
        result.width = ShapeConstants::kMinCircleSize;
      }
      if (result.height < kMinSizeThreshold) {
        result.height = result.width;  // Circle must be square
      }
      break;
    }
    case ShapeModel::ShapeType::Rectangle:
    case ShapeModel::ShapeType::Ellipse: {
      if (result.width < kMinSizeThreshold) {
        result.width = ShapeConstants::kMinRectangleWidth;
      }
      if (result.height < kMinSizeThreshold) {
        result.height = ShapeConstants::kMinRectangleHeight;
      }
      break;
    }
    case ShapeModel::ShapeType::Stick: {
      if (result.width < kMinSizeThreshold) {
        result.width = ShapeConstants::kMinRectangleWidth;
      }
      // Thickness is fixed, don't change it
      break;
    }
    default:
      break;
  }

  return result;
}
