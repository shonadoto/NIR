#pragma once

#include <QGraphicsEllipseItem>
#include <functional>

#include "scene/ISceneObject.h"

class CircleItem : public QGraphicsEllipseItem, public ISceneObject {
 public:
  explicit CircleItem(qreal radius, QGraphicsItem* parent = nullptr);
  ~CircleItem() override = default;

  QWidget* create_properties_widget(QWidget* parent) override;
  QJsonObject to_json() const override;
  void from_json(const QJsonObject& json) override;
  QString type_name() const override {
    return QStringLiteral("circle");
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
  QString name_{"Circle"};
  std::function<void()> geometry_changed_callback_;
  class MaterialModel* material_model_{nullptr};

  void notify_geometry_changed() const;
  void draw_radial_grid(QPainter* painter, const QRectF& extendedRect,
                        const QRectF& baseRect) const;
};
