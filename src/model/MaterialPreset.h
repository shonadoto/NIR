#pragma once

#include <QColor>
#include <QString>
#include <QJsonObject>

/**
 * @brief Represents a material preset with properties that can be applied to shapes.
 *
 * Material presets contain properties like fill color that are independent of the
 * geometric properties of shapes (size, position, rotation).
 */
class MaterialPreset {
public:
  explicit MaterialPreset(const QString &name = QStringLiteral("New Material"));
  ~MaterialPreset() = default;

  QString name() const { return name_; }
  void set_name(const QString &name);

  QColor fill_color() const { return fill_color_; }
  void set_fill_color(const QColor &color) { fill_color_ = color; }

  // Serialization
  QJsonObject to_json() const;
  void from_json(const QJsonObject &json);

  // Comparison
  bool operator==(const MaterialPreset &other) const;
  bool operator!=(const MaterialPreset &other) const { return !(*this == other); }

private:
  QString name_;
  QColor fill_color_{128, 128, 128, 128}; // Default gray with transparency
};

