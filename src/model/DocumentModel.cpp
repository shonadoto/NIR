#include "model/DocumentModel.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "model/MaterialModel.h"
#include "model/ShapeModel.h"
#include "model/SubstrateModel.h"
#include "model/core/ModelTypes.h"

DocumentModel::DocumentModel()
    : substrate_(std::make_shared<SubstrateModel>()) {
  substrate_->on_changed().connect(
    [this](const ModelChange& change) { notify_all(change); });
}

auto DocumentModel::create_shape(ShapeModel::ShapeType type,
                                 const std::string& name)
  -> std::shared_ptr<ShapeModel> {
  auto shape = std::make_shared<ShapeModel>(type);
  if (!name.empty()) {
    shape->set_name(name);
  }
  shape->on_changed().connect(
    [this](const ModelChange& change) { notify_all(change); });
  shapes_.push_back(shape);
  notify_all(ModelChange{ModelChange::Type::Custom, "shape_added"});
  return shape;
}

void DocumentModel::remove_shape(const std::shared_ptr<ShapeModel>& shape) {
  shapes_.erase(std::remove(shapes_.begin(), shapes_.end(), shape),
                shapes_.end());
  notify_all(ModelChange{ModelChange::Type::Custom, "shape_removed"});
}

void DocumentModel::clear_shapes() {
  shapes_.clear();
  notify_all(ModelChange{ModelChange::Type::Custom, "shapes_cleared"});
}

auto DocumentModel::create_material(const Color& color, const std::string& name)
  -> std::shared_ptr<MaterialModel> {
  auto material = std::make_shared<MaterialModel>(color);
  if (!name.empty()) {
    material->set_name(name);
  }
  material->on_changed().connect(
    [this](const ModelChange& change) { notify_all(change); });
  materials_.push_back(material);
  notify_all(ModelChange{ModelChange::Type::Custom, "material_added"});
  return material;
}

void DocumentModel::set_substrate(
  const std::shared_ptr<SubstrateModel>& substrate) {
  if (!substrate) {
    return;  // Invalid substrate, ignore
  }
  substrate_ = substrate;
  substrate_->on_changed().connect(
    [this](const ModelChange& change) { notify_all(change); });
  notify_all(ModelChange{ModelChange::Type::Custom, "substrate_changed"});
}

void DocumentModel::remove_material(
  const std::shared_ptr<MaterialModel>& material) {
  materials_.erase(std::remove(materials_.begin(), materials_.end(), material),
                   materials_.end());
  notify_all(ModelChange{ModelChange::Type::Custom, "material_removed"});
}

void DocumentModel::clear_materials() {
  materials_.clear();
  notify_all(ModelChange{ModelChange::Type::Custom, "materials_cleared"});
}

void DocumentModel::notify_all(const ModelChange& change) {
  changed_signal_.emit_signal(change);
}
