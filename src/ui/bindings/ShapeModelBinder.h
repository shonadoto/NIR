#pragma once

#include <memory>
#include <unordered_map>

#include "model/DocumentModel.h"
#include "model/MaterialModel.h"
#include "model/ShapeModel.h"

class ISceneObject;
class QGraphicsItem;

/**
 * @brief Binds ShapeModel instances to existing scene objects
 * (ISceneObject/QGraphicsItem) and keeps properties (name, color/material) in
 * sync.
 */
class ShapeModelBinder {
 public:
  explicit ShapeModelBinder(DocumentModel& document);

  auto bind_shape(ISceneObject* item) -> std::shared_ptr<ShapeModel>;
  auto attach_shape(ISceneObject* item,
                    const std::shared_ptr<ShapeModel>& model)
    -> std::shared_ptr<ShapeModel>;
  auto model_for(ISceneObject* item) const -> std::shared_ptr<ShapeModel>;
  auto item_for(const std::shared_ptr<ShapeModel>& model) const
    -> QGraphicsItem*;
  void unbind_shape(ISceneObject* item);
  void clear_bindings();

 private:
  struct Binding {
    std::shared_ptr<ShapeModel> model;
    int connection_id{0};
    std::shared_ptr<MaterialModel> bound_material{};
    int material_connection_id{0};
    bool suppress_geometry_callback{false};
    bool suppress_model_geometry_signal{false};
  };

  void handle_change(ISceneObject* item, const ModelChange& change);
  static void apply_name(ISceneObject* item, const std::string& name);
  static void apply_color(ISceneObject* item, const Color& color);
  static auto extract_color(const ISceneObject* item) -> Color;
  void update_material_binding(ISceneObject* item, Binding& binding);
  static void detach_material_binding(Binding& binding);
  void update_model_geometry(ISceneObject* item,
                             const std::shared_ptr<ShapeModel>& model);
  void apply_geometry(ISceneObject* item,
                      const std::shared_ptr<ShapeModel>& model);
  void on_item_geometry_changed(ISceneObject* item);

  DocumentModel& document_;
  std::unordered_map<ISceneObject*, Binding> bindings_;
};
