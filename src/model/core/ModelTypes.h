#pragma once

#include <cstdint>
#include <string>

struct Color {
  uint8_t r{128};
  uint8_t g{128};
  uint8_t b{128};
  uint8_t a{255};

  bool operator==(const Color& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
  }
  bool operator!=(const Color& other) const {
    return !(*this == other);
  }
};

struct Point2D {
  double x{0.0};
  double y{0.0};

  bool operator==(const Point2D& other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const Point2D& other) const {
    return !(*this == other);
  }
};

struct Size2D {
  double width{0.0};
  double height{0.0};

  bool operator==(const Size2D& other) const {
    return width == other.width && height == other.height;
  }
  bool operator!=(const Size2D& other) const {
    return !(*this == other);
  }
};

struct ModelChange {
  enum class Type {
    NameChanged,
    ColorChanged,
    MaterialChanged,
    SizeChanged,
    GeometryChanged,
    Custom
  };

  Type type{Type::Custom};
  std::string property;
};
