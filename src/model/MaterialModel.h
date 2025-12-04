#pragma once

#include "model/core/ModelObject.h"
#include "model/core/ModelTypes.h"

class MaterialModel : public ModelObject {
public:
    enum class GridType {
        None,      // No grid
        Internal   // Internal grid (fills the object)
    };

    explicit MaterialModel(const Color &color = {});

    Color color() const { return color_; }
    void set_color(const Color &color);

    GridType grid_type() const { return grid_type_; }
    void set_grid_type(GridType type);

    // For rectangle: x = horizontal cells, y = vertical cells
    // For circle/ellipse: x = radial lines, y = concentric circles
    double grid_frequency_x() const { return grid_frequency_x_; }
    void set_grid_frequency_x(double frequency);
    double grid_frequency_y() const { return grid_frequency_y_; }
    void set_grid_frequency_y(double frequency);

    // Legacy method for backward compatibility
    double grid_frequency() const { return grid_frequency_x_; }
    void set_grid_frequency(double frequency) { set_grid_frequency_x(frequency); }

private:
    Color color_;
    GridType grid_type_ {GridType::None};
    double grid_frequency_x_ {5.0}; // Horizontal cells (rectangle) or radial lines (circle/ellipse)
    double grid_frequency_y_ {5.0}; // Vertical cells (rectangle) or concentric circles (circle/ellipse)
};

