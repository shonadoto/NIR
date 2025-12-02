#pragma once

#include "model/core/ModelObject.h"
#include "model/core/ModelTypes.h"

class MaterialModel : public ModelObject {
public:
    enum class GridType {
        None,      // No grid
        Radial,    // Radial grid (converges to center) - for circles and ellipses
        Internal   // Internal grid (fills the object) - for future use
    };

    explicit MaterialModel(const Color &color = {});

    Color color() const { return color_; }
    void set_color(const Color &color);

    GridType grid_type() const { return grid_type_; }
    void set_grid_type(GridType type);

    double grid_frequency() const { return grid_frequency_; }
    void set_grid_frequency(double frequency);

private:
    Color color_;
    GridType grid_type_ {GridType::None};
    double grid_frequency_ {10.0}; // Grid lines per unit (e.g., per 100px)
};

