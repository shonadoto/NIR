#pragma once

#include "model/core/ModelObject.h"
#include "model/core/ModelTypes.h"

class SubstrateModel : public ModelObject {
public:
    SubstrateModel(const Size2D &size = {}, const Color &color = {});

    Size2D size() const { return size_; }
    void set_size(const Size2D &size);

    Color color() const { return color_; }
    void set_color(const Color &color);

private:
    Size2D size_;
    Color color_;
};

