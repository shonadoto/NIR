#pragma once

#include <QGraphicsLineItem>

#include "scene/ISceneObject.h"
#include "scene/items/BaseShapeItem.h"

class StickItem : public BaseShapeItem<QGraphicsLineItem> {
 public:
  explicit StickItem(const QLineF& line, QGraphicsItem* parent = nullptr);
  ~StickItem() override = default;

  QWidget* create_properties_widget(QWidget* parent) override;
  QJsonObject to_json() const override;
  void from_json(const QJsonObject& json) override;
  QString type_name() const override {
    return QStringLiteral("stick");
  }
  // name(), set_name(), set_geometry_changed_callback(),
  // clear_geometry_changed_callback(), set_material_model() inherited from
  // BaseShapeItem

 protected:
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) override;
};
