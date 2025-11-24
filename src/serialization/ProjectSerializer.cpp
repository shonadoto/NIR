#include "ProjectSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
        case ShapeModel::ShapeType::Rectangle: return QStringLiteral("rectangle");
        case ShapeModel::ShapeType::Ellipse: return QStringLiteral("ellipse");
        case ShapeModel::ShapeType::Circle: return QStringLiteral("circle");
        case ShapeModel::ShapeType::Stick: return QStringLiteral("stick");
    }
    return QStringLiteral("rectangle");
}

ShapeModel::ShapeType shape_type_from_string(const QString &value) {
    if (value == QLatin1String("ellipse")) return ShapeModel::ShapeType::Ellipse;
    if (value == QLatin1String("circle")) return ShapeModel::ShapeType::Circle;
    if (value == QLatin1String("stick")) return ShapeModel::ShapeType::Stick;
    return ShapeModel::ShapeType::Rectangle;
}

QJsonArray color_to_json(const Color &color) {
    return QJsonArray{color.r, color.g, color.b, color.a};
}

Color color_from_json(const QJsonArray &array) {
    Color color;
    if (array.size() == 4) {
        color.r = static_cast<uint8_t>(array[0].toInt());
        color.g = static_cast<uint8_t>(array[1].toInt());
        color.b = static_cast<uint8_t>(array[2].toInt());
        color.a = static_cast<uint8_t>(array[3].toInt());
    }
    return color;
}

QJsonObject point_to_json(const Point2D &point) {
    return QJsonObject{
        {"x", point.x},
        {"y", point.y}
    };
}

Point2D point_from_json(const QJsonObject &object) {
    Point2D point;
    point.x = object["x"].toDouble();
    point.y = object["y"].toDouble();
    return point;
}

QJsonObject size_to_json(const Size2D &size) {
    return QJsonObject{
        {"width", size.width},
        {"height", size.height}
    };
}

Size2D size_from_json(const QJsonObject &object) {
    Size2D size;
    size.width = object["width"].toDouble();
    size.height = object["height"].toDouble();
    return size;
}

Point2D position_from_value(const QJsonValue &value) {
    if (value.isObject()) {
        return point_from_json(value.toObject());
    }
    if (value.isArray()) {
        QJsonArray arr = value.toArray();
        if (arr.size() >= 2) {
            return Point2D{arr[0].toDouble(), arr[1].toDouble()};
        }
    }
    return Point2D{};
}

Size2D size_from_value(const QJsonObject &object, ShapeModel::ShapeType type) {
    if (object.contains("size")) {
        return size_from_json(object["size"].toObject());
    }
    if (object.contains("width") && object.contains("height")) {
        return Size2D{object["width"].toDouble(), object["height"].toDouble()};
    }
    if (type == ShapeModel::ShapeType::Circle && object.contains("radius")) {
        double radius = object["radius"].toDouble();
        double diameter = radius * 2.0;
        return Size2D{diameter, diameter};
    }
    if (type == ShapeModel::ShapeType::Stick && object.contains("line")) {
        QJsonObject line = object["line"].toObject();
        double dx = line["x2"].toDouble() - line["x1"].toDouble();
        double dy = line["y2"].toDouble() - line["y1"].toDouble();
        double length = std::sqrt(dx * dx + dy * dy);
        double width = object.contains("pen_width") ? object["pen_width"].toDouble() : 2.0;
        return Size2D{length, width};
    }
    return Size2D{100.0, 100.0};
}

Color custom_color_from_object(const QJsonObject &object) {
    if (object.contains("custom_color")) {
        return color_from_json(object["custom_color"].toArray());
    }
    if (object.contains("fill_color")) {
        return color_from_json(object["fill_color"].toArray());
    }
    return Color{};
}

} // namespace

bool ProjectSerializer::save_to_file(const QString &filename, DocumentModel *document) {
    LOG_INFO() << "Saving project to: " << filename.toStdString();

    if (!document) {
        LOG_ERROR() << "Save failed: document is null";
        return false;
    }

    QJsonObject root;
    root["version"] = kVersion;

    if (auto substrate = document->substrate()) {
        const Size2D size = substrate->size();
        QJsonObject substrateObj;
        substrateObj["name"] = QString::fromStdString(substrate->name());
        substrateObj["size"] = size_to_json(size);
        substrateObj["color"] = color_to_json(substrate->color());
        root["substrate"] = substrateObj;
    }

    QJsonArray materials;
    for (const auto &material : document->materials()) {
        if (!material) {
            continue;
        }
        QJsonObject materialObj;
        materialObj["name"] = QString::fromStdString(material->name());
        materialObj["color"] = color_to_json(material->color());
        materials.append(materialObj);
    }
    root["materials"] = materials;
    root["material_presets"] = materials; // backward compatibility

    QJsonArray shapes;
    for (const auto &shape : document->shapes()) {
        if (!shape) {
            continue;
        }
        QJsonObject obj;
        obj["name"] = QString::fromStdString(shape->name());
        obj["type"] = to_string(shape->type());
        obj["position"] = point_to_json(shape->position());
        obj["size"] = size_to_json(shape->size());
        obj["rotation"] = shape->rotation_deg();
        obj["material_mode"] = shape->material_mode() == ShapeModel::MaterialMode::Preset
            ? QStringLiteral("preset")
            : QStringLiteral("custom");
        if (shape->material()) {
            obj["material_name"] = QString::fromStdString(shape->material()->name());
        } else {
            obj["custom_color"] = color_to_json(shape->custom_color());
        }
        shapes.append(obj);
    }
    root["objects"] = shapes;

    QJsonDocument doc(root);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR() << "Failed to open file for writing: " << filename.toStdString();
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    LOG_INFO() << "Project saved successfully: " << filename.toStdString();
    return true;
}

bool ProjectSerializer::load_from_file(const QString &filename, DocumentModel *document) {
    LOG_INFO() << "Loading project from: " << filename.toStdString();

    if (!document) {
        LOG_ERROR() << "Load failed: document is null";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR() << "Failed to open file for reading: " << filename.toStdString();
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
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
        QJsonObject substrateObj = root["substrate"].toObject();
        auto substrate = std::make_shared<SubstrateModel>();
        substrate->set_name(substrateObj["name"].toString(QStringLiteral("Substrate")).toStdString());
        if (substrateObj.contains("size")) {
            substrate->set_size(size_from_json(substrateObj["size"].toObject()));
        } else if (substrateObj.contains("width") && substrateObj.contains("height")) {
            substrate->set_size(Size2D{
                substrateObj["width"].toDouble(),
                substrateObj["height"].toDouble()
            });
        }
        if (substrateObj.contains("color")) {
            substrate->set_color(color_from_json(substrateObj["color"].toArray()));
        } else if (substrateObj.contains("fill_color")) {
            substrate->set_color(color_from_json(substrateObj["fill_color"].toArray()));
        }
        document->set_substrate(substrate);
    }

    std::unordered_map<std::string, std::shared_ptr<MaterialModel>> materials_by_name;
    QJsonArray materials = root.contains("materials")
        ? root["materials"].toArray()
        : root["material_presets"].toArray();
    for (const QJsonValue &value : materials) {
        QJsonObject materialObj = value.toObject();
        QString name = materialObj["name"].toString(QStringLiteral("Material"));
        Color color = materialObj.contains("color")
            ? color_from_json(materialObj["color"].toArray())
            : color_from_json(materialObj["fill_color"].toArray());
        auto material = document->create_material(color, name.toStdString());
        materials_by_name.emplace(name.toStdString(), material);
    }

    if (root.contains("objects")) {
        QJsonArray shapes = root["objects"].toArray();
        for (const QJsonValue &value : shapes) {
            QJsonObject obj = value.toObject();
            QString name = obj["name"].toString(QStringLiteral("Shape"));
            ShapeModel::ShapeType type = shape_type_from_string(obj["type"].toString(QStringLiteral("rectangle")));
            auto shape = document->create_shape(type, name.toStdString());

            if (obj.contains("position")) {
                shape->set_position(position_from_value(obj["position"]));
            }
            shape->set_size(size_from_value(obj, type));
            if (obj.contains("rotation")) {
                shape->set_rotation_deg(obj["rotation"].toDouble());
            }
            QString mode = obj["material_mode"].toString(QStringLiteral("custom"));
            if (mode == QLatin1String("preset") && obj.contains("material_name")) {
                const std::string matName = obj["material_name"].toString().toStdString();
                auto it = materials_by_name.find(matName);
                if (it != materials_by_name.end()) {
                    shape->assign_material(it->second);
                } else {
                    shape->clear_material();
                }
            } else if (obj.contains("custom_color") || obj.contains("fill_color")) {
                shape->clear_material();
                shape->set_custom_color(custom_color_from_object(obj));
            } else {
                shape->clear_material();
            }
        }
    }

    LOG_INFO() << "Project loaded successfully: " << filename.toStdString();
    return true;
}
