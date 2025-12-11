#include "ProjectSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>

#include "model/DocumentModel.h"
#include "model/MaterialModel.h"
#include "model/ShapeModel.h"
#include "model/SubstrateModel.h"
#include "model/core/ModelTypes.h"
#include "utils/Logging.h"

namespace {

constexpr auto kVersion = "2.1";

QString to_string(ShapeModel::ShapeType type) {
  switch (type) {
    case ShapeModel::ShapeType::Rectangle:
      return QStringLiteral("rectangle");
    case ShapeModel::ShapeType::Ellipse:
      return QStringLiteral("ellipse");
    case ShapeModel::ShapeType::Circle:
      return QStringLiteral("circle");
    case ShapeModel::ShapeType::Stick:
      return QStringLiteral("stick");
  }
  return QStringLiteral("rectangle");
}

ShapeModel::ShapeType shape_type_from_string(const QString& value) {
  if (value == QLatin1String("ellipse")) {
    return ShapeModel::ShapeType::Ellipse;
  }
  if (value == QLatin1String("circle")) {
    return ShapeModel::ShapeType::Circle;
  }
  if (value == QLatin1String("stick")) {
    return ShapeModel::ShapeType::Stick;
  }
  return ShapeModel::ShapeType::Rectangle;
}

QJsonArray color_to_json(const Color& color) {
  return QJsonArray{color.r, color.g, color.b, color.a};
}

Color color_from_json(const QJsonArray& array) {
  Color color;
  if (array.size() == 4) {
    // Clamp values to valid range [0, 255]
    constexpr int kMaxColorValue = 255;
    const int red = std::clamp(array[0].toInt(), 0, kMaxColorValue);
    const int green = std::clamp(array[1].toInt(), 0, kMaxColorValue);
    const int blue = std::clamp(array[2].toInt(), 0, kMaxColorValue);
    const int alpha = std::clamp(array[3].toInt(), 0, kMaxColorValue);
    color.r = static_cast<uint8_t>(red);
    color.g = static_cast<uint8_t>(green);
    color.b = static_cast<uint8_t>(blue);
    color.a = static_cast<uint8_t>(alpha);
  }
  return color;
}

QJsonObject point_to_json(const Point2D& point) {
  return QJsonObject{{"x", point.x}, {"y", point.y}};
}

Point2D point_from_json(const QJsonObject& object) {
  Point2D point;
  const double pos_x = object["x"].toDouble();
  const double pos_y = object["y"].toDouble();
  // Validate values are not NaN or Infinity
  if (std::isfinite(pos_x) && std::isfinite(pos_y)) {
    point.x = pos_x;
    point.y = pos_y;
  }
  // Otherwise point remains {0, 0}
  return point;
}

QJsonObject size_to_json(const Size2D& size) {
  return QJsonObject{{"width", size.width}, {"height", size.height}};
}

Size2D size_from_json(const QJsonObject& object) {
  Size2D size;
  const double width = object["width"].toDouble();
  const double height = object["height"].toDouble();
  // Validate values are not NaN or Infinity, and are non-negative
  if (std::isfinite(width) && width >= 0.0 && std::isfinite(height) &&
      height >= 0.0) {
    size.width = width;
    size.height = height;
  }
  // Otherwise size remains {0, 0}
  return size;
}

Point2D position_from_value(const QJsonValue& value) {
  if (value.isObject()) {
    return point_from_json(value.toObject());
  }
  if (value.isArray()) {
    QJsonArray arr = value.toArray();
    if (arr.size() >= 2) {
      const double pos_x = arr[0].toDouble();
      const double pos_y = arr[1].toDouble();
      if (std::isfinite(pos_x) && std::isfinite(pos_y)) {
        return Point2D{pos_x, pos_y};
      }
    }
  }
  return Point2D{};
}

Size2D size_from_value(const QJsonObject& object, ShapeModel::ShapeType type) {
  if (object.contains("size")) {
    return size_from_json(object["size"].toObject());
  }
  if (object.contains("width") && object.contains("height")) {
    const double width = object["width"].toDouble();
    const double height = object["height"].toDouble();
    if (std::isfinite(width) && width > 0.0 && std::isfinite(height) &&
        height > 0.0) {
      return Size2D{width, height};
    }
  }
  if (type == ShapeModel::ShapeType::Circle && object.contains("radius")) {
    constexpr double kRadiusToDiameterMultiplier = 2.0;
    const double radius = object["radius"].toDouble();
    if (std::isfinite(radius) && radius > 0.0) {
      const double diameter = radius * kRadiusToDiameterMultiplier;
      if (std::isfinite(diameter)) {
        return Size2D{diameter, diameter};
      }
    }
  }
  if (type == ShapeModel::ShapeType::Stick && object.contains("line")) {
    constexpr double kDefaultPenWidth = 2.0;
    QJsonObject line = object["line"].toObject();
    const double line_x1 = line["x1"].toDouble();
    const double line_y1 = line["y1"].toDouble();
    const double line_x2 = line["x2"].toDouble();
    const double line_y2 = line["y2"].toDouble();
    if (std::isfinite(line_x1) && std::isfinite(line_y1) &&
        std::isfinite(line_x2) && std::isfinite(line_y2)) {
      const double delta_x = line_x2 - line_x1;
      const double delta_y = line_y2 - line_y1;
      const double length = std::sqrt(delta_x * delta_x + delta_y * delta_y);
      if (std::isfinite(length) && length > 0.0) {
        const double width = object.contains("pen_width")
                               ? object["pen_width"].toDouble()
                               : kDefaultPenWidth;
        if (std::isfinite(width) && width > 0.0) {
          return Size2D{length, width};
        }
      }
    }
  }
  return Size2D{100.0, 100.0};
}

Color custom_color_from_object(const QJsonObject& object) {
  if (object.contains("custom_color")) {
    return color_from_json(object["custom_color"].toArray());
  }
  if (object.contains("fill_color")) {
    return color_from_json(object["fill_color"].toArray());
  }
  return Color{};
}

}  // namespace

bool ProjectSerializer::save_to_file(const QString& filename,
                                     DocumentModel* document) {
  LOG_INFO() << "Saving project to: " << filename.toStdString();

  if (document == nullptr) {
    LOG_ERROR() << "Save failed: document is null";
    return false;
  }

  QJsonObject root;
  root["version"] = kVersion;

  if (auto substrate = document->substrate()) {
    const Size2D size = substrate->size();
    QJsonObject substrate_obj;
    substrate_obj["name"] = QString::fromStdString(substrate->name());
    substrate_obj["size"] = size_to_json(size);
    substrate_obj["color"] = color_to_json(substrate->color());
    root["substrate"] = substrate_obj;
  }

  QJsonArray materials;
  for (const auto& material : document->materials()) {
    if (!material) {
      continue;
    }
    QJsonObject material_obj;
    material_obj["name"] = QString::fromStdString(material->name());
    material_obj["color"] = color_to_json(material->color());
    // Save grid settings
    material_obj["grid_type"] = static_cast<int>(material->grid_type());
    material_obj["grid_frequency_x"] = material->grid_frequency_x();
    material_obj["grid_frequency_y"] = material->grid_frequency_y();
    // Backward compatibility
    material_obj["grid_frequency"] = material->grid_frequency_x();
    materials.append(material_obj);
  }
  root["materials"] = materials;
  root["material_presets"] = materials;  // backward compatibility

  QJsonArray shapes;
  for (const auto& shape : document->shapes()) {
    if (!shape) {
      continue;
    }
    QJsonObject obj;
    obj["name"] = QString::fromStdString(shape->name());
    obj["type"] = to_string(shape->type());
    obj["position"] = point_to_json(shape->position());
    obj["size"] = size_to_json(shape->size());
    obj["rotation"] = shape->rotation_deg();
    obj["material_mode"] =
      shape->material_mode() == ShapeModel::MaterialMode::Preset
        ? QStringLiteral("preset")
        : QStringLiteral("custom");
    if (shape->material_mode() == ShapeModel::MaterialMode::Preset &&
        shape->material()) {
      obj["material_name"] = QString::fromStdString(shape->material()->name());
    } else if (shape->material()) {
      // Custom material - save color and grid settings
      obj["custom_color"] = color_to_json(shape->material()->color());
      obj["grid_type"] = static_cast<int>(shape->material()->grid_type());
      obj["grid_frequency_x"] = shape->material()->grid_frequency_x();
      obj["grid_frequency_y"] = shape->material()->grid_frequency_y();
      // Backward compatibility
      obj["grid_frequency"] = shape->material()->grid_frequency_x();
    }
    shapes.append(obj);
  }
  root["objects"] = shapes;

  const QJsonDocument doc(root);
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    LOG_ERROR() << "Failed to open file for writing: "
                << filename.toStdString();
    return false;
  }
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();
  LOG_INFO() << "Project saved successfully: " << filename.toStdString();
  return true;
}

bool ProjectSerializer::load_from_file(const QString& filename,
                                       DocumentModel* document) {
  LOG_INFO() << "Loading project from: " << filename.toStdString();

  if (document == nullptr) {
    LOG_ERROR() << "Load failed: document is null";
    return false;
  }

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    LOG_ERROR() << "Failed to open file for reading: "
                << filename.toStdString();
    return false;
  }
  const QByteArray data = file.readAll();
  file.close();

  const QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || !doc.isObject()) {
    LOG_ERROR() << "Invalid project file format";
    return false;
  }

  QJsonObject root = doc.object();
  if (root["version"].toString() != kVersion) {
    LOG_WARN() << "Loading project with version mismatch";
  }

  document->clear_shapes();
  document->clear_materials();

  if (root.contains("substrate")) {
    QJsonObject substrate_obj = root["substrate"].toObject();
    auto substrate = std::make_shared<SubstrateModel>();
    substrate->set_name(substrate_obj["name"]
                          .toString(QStringLiteral("Substrate"))
                          .toStdString());
    if (substrate_obj.contains("size")) {
      const Size2D size = size_from_json(substrate_obj["size"].toObject());
      if (size.width > 0.0 && size.height > 0.0) {
        substrate->set_size(size);
      }
    } else if (substrate_obj.contains("width") &&
               substrate_obj.contains("height")) {
      const double width = substrate_obj["width"].toDouble();
      const double height = substrate_obj["height"].toDouble();
      if (std::isfinite(width) && width > 0.0 && std::isfinite(height) &&
          height > 0.0) {
        substrate->set_size(Size2D{width, height});
      }
    }
    if (substrate_obj.contains("color")) {
      substrate->set_color(color_from_json(substrate_obj["color"].toArray()));
    } else if (substrate_obj.contains("fill_color")) {
      substrate->set_color(
        color_from_json(substrate_obj["fill_color"].toArray()));
    }
    document->set_substrate(substrate);
  }

  std::unordered_map<std::string, std::shared_ptr<MaterialModel>>
    materials_by_name;
  const QJsonArray materials = root.contains("materials")
                                 ? root["materials"].toArray()
                                 : root["material_presets"].toArray();
  for (const auto& value : materials) {
    QJsonObject material_obj = value.toObject();
    const QString name =
      material_obj["name"].toString(QStringLiteral("Material"));
    const Color color =
      material_obj.contains("color")
        ? color_from_json(material_obj["color"].toArray())
        : color_from_json(material_obj["fill_color"].toArray());
    auto material = document->create_material(color, name.toStdString());

    // Load grid settings
    if (material_obj.contains("grid_type")) {
      const int grid_type_value = material_obj["grid_type"].toInt();
      // Handle backward compatibility: old Radial (1) becomes Internal (1)
      auto grid_type = static_cast<MaterialModel::GridType>(grid_type_value);
      // Ensure only valid values (0 or 1)
      if (grid_type_value > 1) {
        grid_type = MaterialModel::GridType::None;
      }
      material->set_grid_type(grid_type);
    }
    // Load grid frequency (new format with x and y, or old format for backward
    // compatibility)
    if (material_obj.contains("grid_frequency_x") &&
        material_obj.contains("grid_frequency_y")) {
      const double freq_x = material_obj["grid_frequency_x"].toDouble();
      const double freq_y = material_obj["grid_frequency_y"].toDouble();
      // Validate values are finite and positive
      if (std::isfinite(freq_x) && freq_x > 0.0) {
        material->set_grid_frequency_x(freq_x);
      }
      if (std::isfinite(freq_y) && freq_y > 0.0) {
        material->set_grid_frequency_y(freq_y);
      }
    } else if (material_obj.contains("grid_frequency")) {
      // Backward compatibility: use old single value for both x and y
      const double freq = material_obj["grid_frequency"].toDouble();
      if (std::isfinite(freq) && freq > 0.0) {
        material->set_grid_frequency_x(freq);
        material->set_grid_frequency_y(freq);
      }
    }

    materials_by_name.emplace(name.toStdString(), material);
  }

  if (root.contains("objects")) {
    const QJsonArray shapes = root["objects"].toArray();
    for (const auto& value : shapes) {
      QJsonObject obj = value.toObject();
      const QString name = obj["name"].toString(QStringLiteral("Shape"));
      const ShapeModel::ShapeType type = shape_type_from_string(
        obj["type"].toString(QStringLiteral("rectangle")));
      auto shape = document->create_shape(type, name.toStdString());

      if (obj.contains("position")) {
        const Point2D pos = position_from_value(obj["position"]);
        if (std::isfinite(pos.x) && std::isfinite(pos.y)) {
          shape->set_position(pos);
        }
      }
      const Size2D size = size_from_value(obj, type);
      if (size.width > 0.0 && size.height > 0.0) {
        shape->set_size(size);
      }
      if (obj.contains("rotation")) {
        const double rotation = obj["rotation"].toDouble();
        if (std::isfinite(rotation)) {
          shape->set_rotation_deg(rotation);
        }
      }
      const QString mode =
        obj["material_mode"].toString(QStringLiteral("custom"));
      if (mode == QLatin1String("preset") && obj.contains("material_name")) {
        const std::string mat_name =
          obj["material_name"].toString().toStdString();
        auto material_iterator = materials_by_name.find(mat_name);
        if (material_iterator != materials_by_name.end()) {
          shape->assign_material(material_iterator->second);
        } else {
          shape->clear_material();
        }
      } else {
        // Custom material
        shape->clear_material();
        // clear_material() creates a new material, so material() should not be
        // nullptr but check for safety
        if (auto* material = shape->material().get()) {
          if (obj.contains("custom_color") || obj.contains("fill_color")) {
            material->set_color(custom_color_from_object(obj));
          }
          // Load grid settings
          if (obj.contains("grid_type")) {
            const int grid_type_value = obj["grid_type"].toInt();
            // Handle backward compatibility: old Radial (1) becomes Internal
            // (1) None = 0, Internal = 1 (Radial was also 1, so it maps
            // correctly)
            auto grid_type =
              static_cast<MaterialModel::GridType>(grid_type_value);
            // Ensure only valid values (0 or 1)
            if (grid_type_value > 1) {
              grid_type = MaterialModel::GridType::None;
            }
            material->set_grid_type(grid_type);
          }
          // Load grid frequency (new format with x and y, or old format for
          // backward compatibility)
          if (obj.contains("grid_frequency_x") &&
              obj.contains("grid_frequency_y")) {
            const double freq_x = obj["grid_frequency_x"].toDouble();
            const double freq_y = obj["grid_frequency_y"].toDouble();
            // Validate values are finite and positive
            if (std::isfinite(freq_x) && freq_x > 0.0) {
              material->set_grid_frequency_x(freq_x);
            }
            if (std::isfinite(freq_y) && freq_y > 0.0) {
              material->set_grid_frequency_y(freq_y);
            }
          } else if (obj.contains("grid_frequency")) {
            // Backward compatibility: use old single value for both x and y
            const double freq = obj["grid_frequency"].toDouble();
            if (std::isfinite(freq) && freq > 0.0) {
              material->set_grid_frequency_x(freq);
              material->set_grid_frequency_y(freq);
            }
          }
        }
      }
    }
  }

  LOG_INFO() << "Project loaded successfully: " << filename.toStdString();
  return true;
}
