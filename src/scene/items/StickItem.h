#pragma once

#include <QGraphicsLineItem>
#include <functional>

#include "scene/ISceneObject.h"

class StickItem : public QGraphicsLineItem, public ISceneObject {
 public:
  explicit StickItem(const QLineF& line, QGraphicsItem* parent = nullptr);
  ~StickItem() override = default;

  QWidget* create_properties_widget(QWidget* parent) override;
  QJsonObject to_json() const override;
  void from_json(const QJsonObject& json) override;
  QString type_name() const override {
    return QStringLiteral("stick");
  }
  QString name() const override {
    return name_;
  }
  void set_name(const QString& name) override;
  void set_geometry_changed_callback(std::function<void()> callback) override;
  void clear_geometry_changed_callback() override;
  void set_material_model(class MaterialModel* material) override;

 protected:
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) override;

 private:
  QString name_{"Stick"};
  std::function<void()> geometry_changed_callback_;

  void notify_geometry_changed() const;
};
