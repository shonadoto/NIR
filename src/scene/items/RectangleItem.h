#pragma once

#include <QGraphicsRectItem>

#include "scene/ISceneObject.h"
#include "scene/items/BaseShapeItem.h"

class RectangleItem : public BaseShapeItem<QGraphicsRectItem> {
 public:
  explicit RectangleItem(const QRectF& rect, QGraphicsItem* parent = nullptr);
  ~RectangleItem() override = default;

  // ISceneObject interface
  QWidget* create_properties_widget(QWidget* parent) override;
  QJsonObject to_json() const override;
  void from_json(const QJsonObject& json) override;
  QString type_name() const override {
    return QStringLiteral("rectangle");
  }
  // name(), set_name(), set_geometry_changed_callback(),
  // clear_geometry_changed_callback(), set_material_model() inherited from
  // BaseShapeItem

 protected:
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) override;

 private:
  void draw_internal_grid(QPainter* painter, const QRectF& rect) const;
};
