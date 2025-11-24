#include "MaterialPreset.h"

#include <QJsonArray>
#include <QJsonObject>

MaterialPreset::MaterialPreset(const QString &name) : name_(name.isEmpty() ? QStringLiteral("New Material") : name) {}

void MaterialPreset::set_name(const QString &name) {
  if (!name.trimmed().isEmpty()) {
    name_ = name;
  }
}

QJsonObject MaterialPreset::to_json() const {
  QJsonObject obj;
  obj["name"] = name_;
  QColor c = fill_color_;
  obj["fill_color"] = QJsonArray{c.red(), c.green(), c.blue(), c.alpha()};
  return obj;
}

void MaterialPreset::from_json(const QJsonObject &json) {
  if (json.contains("name")) {
    QString loadedName = json["name"].toString().trimmed();
    if (!loadedName.isEmpty()) {
      name_ = loadedName;
    }
  }
  if (json.contains("fill_color")) {
    QJsonArray c = json["fill_color"].toArray();
    if (c.size() >= 4) {
      fill_color_ = QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt());
    } else if (c.size() >= 3) {
      fill_color_ = QColor(c[0].toInt(), c[1].toInt(), c[2].toInt());
    }
  }
}

bool MaterialPreset::operator==(const MaterialPreset &other) const {
  return name_ == other.name_ && fill_color_ == other.fill_color_;
}

