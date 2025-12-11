#pragma once

#include <QGraphicsItem>
#include <QString>
#include <functional>

#include "scene/ISceneObject.h"

class MaterialModel;

/**
 * @brief Base class for all shape items providing common ISceneObject
 * functionality.
 *
 * This template class provides common implementation for:
 * - Name management
 * - Geometry change callbacks
 * - Material model storage
 * - Common itemChange handling
 *
 * @tparam BaseGraphicsItem The Qt graphics item base class (QGraphicsRectItem,
 * QGraphicsEllipseItem, QGraphicsLineItem, etc.)
 */
template <typename BaseGraphicsItem>
class BaseShapeItem : public BaseGraphicsItem, public ISceneObject {
 public:
  using BaseGraphicsItem::BaseGraphicsItem;

  // ISceneObject interface
  QString name() const override {
    return name_;
  }

  void set_name(const QString& name) override {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || trimmed == name_) {
      return;
    }
    name_ = trimmed;
  }

  void set_geometry_changed_callback(std::function<void()> callback) override {
    geometry_changed_callback_ = std::move(callback);
  }

  void clear_geometry_changed_callback() override {
    geometry_changed_callback_ = nullptr;
  }

  void set_material_model(MaterialModel* material) override {
    material_model_ = material;
    BaseGraphicsItem::update();  // Trigger repaint to show/hide grid
  }

 protected:
  /**
   * @brief Notify that geometry has changed.
   * Should be called from itemChange when position/size/rotation changes.
   */
  void notify_geometry_changed() const {
    if (geometry_changed_callback_) {
      geometry_changed_callback_();
    }
  }

  /**
   * @brief Get the material model for grid rendering.
   */
  MaterialModel* material_model() const {
    return material_model_;
  }

  /**
   * @brief Common itemChange implementation that notifies on geometry changes.
   * Derived classes should call this from their itemChange override.
   */
  void handle_geometry_change(QGraphicsItem::GraphicsItemChange change) {
    if (change == QGraphicsItem::ItemPositionHasChanged ||
        change == QGraphicsItem::ItemRotationHasChanged ||
        change == QGraphicsItem::ItemTransformHasChanged) {
      notify_geometry_changed();
    }
  }

 private:
  QString name_;
  std::function<void()> geometry_changed_callback_;
  MaterialModel* material_model_{nullptr};
};
