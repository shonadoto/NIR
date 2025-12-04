#pragma once

#include <QColor>

#include "model/core/ModelTypes.h"

inline QColor to_qcolor(const Color& color) {
  return QColor(color.r, color.g, color.b, color.a);
}

inline Color to_model_color(const QColor& color) {
  return Color{
    static_cast<uint8_t>(color.red()), static_cast<uint8_t>(color.green()),
    static_cast<uint8_t>(color.blue()), static_cast<uint8_t>(color.alpha())};
}
