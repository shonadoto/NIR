#pragma once

#include "model/core/ModelTypes.h"
#include "model/ShapeModel.h"

namespace ShapeConstants {
    constexpr double kStickThickness = 2.0;
    constexpr double kMinCircleSize = 80.0;
    constexpr double kMinRectangleWidth = 100.0;
    constexpr double kMinRectangleHeight = 60.0;
}

class ShapeSizeConverter {
public:
    // Convert size between different shape types
    static Size2D convert(ShapeModel::ShapeType from, ShapeModel::ShapeType to, const Size2D &size);
    
    // Ensure minimum size for a given shape type
    static Size2D ensure_minimum(const Size2D &size, ShapeModel::ShapeType type);
};

