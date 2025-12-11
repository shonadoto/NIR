#pragma once

#include <QGraphicsItem>
#include <functional>

#include "scene/ISceneObject.h"

class SubstrateItem : public QGraphicsItem, public ISceneObject {
 public:
  explicit SubstrateItem(const QSizeF& size);

  auto boundingRect() const -> QRectF override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  QSizeF size() const {
    return size_;
  }
  void set_size(const QSizeF& size);

  QColor fill_color() const {
    return fill_color_;
  }
  void set_fill_color(const QColor& color);

  // ISceneObject interface
  auto create_properties_widget(QWidget* parent) -> QWidget* override;
  QJsonObject to_json() const override;
  void from_json(const QJsonObject& json) override;
  QString type_name() const override {
    return QStringLiteral("substrate");
  }
  QString name() const override {
    return name_;
  }
  void set_name(const QString& name) override;
  void set_geometry_changed_callback(std::function<void()> callback) override {
    geometry_callback_ = std::move(callback);
  }
  void clear_geometry_changed_callback() override {
    geometry_callback_ = nullptr;
  }
  void set_material_model(class MaterialModel*) override {}

 private:
  QSizeF size_;
  QColor fill_color_{240, 240, 240};
  QString name_{"Substrate"};
  std::function<void()> geometry_callback_;
};
