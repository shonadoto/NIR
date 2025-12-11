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

  /**
   * @brief Check if color values are valid (all in range 0-255).
   * @return true if color is valid.
   * @note Since color components are uint8_t, they are always in valid range.
   *       This method is kept for API consistency with Size2D::is_valid() and
   *       potential future validation needs.
   */
  bool is_valid() const {
    // uint8_t is always in range 0-255, so this is always true
    return true;
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

  Point2D operator+(const Point2D& other) const {
    return Point2D{x + other.x, y + other.y};
  }

  Point2D operator-(const Point2D& other) const {
    return Point2D{x - other.x, y - other.y};
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

  Size2D operator+(const Size2D& other) const {
    return Size2D{width + other.width, height + other.height};
  }

  Size2D operator-(const Size2D& other) const {
    return Size2D{width - other.width, height - other.height};
  }

  /**
   * @brief Check if size is valid (both dimensions >= 0).
   * @return true if size is valid.
   */
  bool is_valid() const {
    return width >= 0.0 && height >= 0.0;
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

  bool operator==(const ModelChange& other) const {
    return type == other.type && property == other.property;
  }
  bool operator!=(const ModelChange& other) const {
    return !(*this == other);
  }
};
