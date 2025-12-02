#pragma once

#include "model/core/ModelObject.h"
#include "model/core/ModelTypes.h"
#include "model/MaterialModel.h"
#include <memory>

class ShapeModel : public ModelObject {
public:
    enum class ShapeType {
        Rectangle,
        Ellipse,
        Circle,
        Stick
    };

    enum class MaterialMode {
        Custom,
        Preset
    };

    explicit ShapeModel(ShapeType type = ShapeType::Rectangle);

    ShapeType type() const { return type_; }
    void set_type(ShapeType type);

    MaterialMode material_mode() const { return material_ ? MaterialMode::Preset : MaterialMode::Custom; }

    std::shared_ptr<MaterialModel> material() const { return material_; }
    void assign_material(const std::shared_ptr<MaterialModel> &material);
    void clear_material();

    Color custom_color() const { return custom_color_; }
    void set_custom_color(const Color &color);

    Point2D position() const { return position_; }
    void set_position(const Point2D &pos);

    Size2D size() const { return size_; }
    void set_size(const Size2D &size);

    double rotation_deg() const { return rotation_deg_; }
    void set_rotation_deg(double rotation);

private:
    ShapeType type_;
    std::shared_ptr<MaterialModel> material_;
    Color custom_color_;
    Point2D position_;
    Size2D size_{100.0, 100.0};
    double rotation_deg_ {0.0};
};

