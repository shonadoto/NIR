#pragma once

#include "model/core/ModelObject.h"
#include "model/core/ModelTypes.h"

class MaterialModel : public ModelObject {
public:
    explicit MaterialModel(const Color &color = {});

    Color color() const { return color_; }
    void set_color(const Color &color);

private:
    Color color_;
};

