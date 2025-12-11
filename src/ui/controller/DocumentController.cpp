#include "ui/controller/DocumentController.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <QSizeF>
#include <memory>

#include "commands/CommandManager.h"
#include "model/DocumentModel.h"
#include "model/ShapeModel.h"
#include "model/ShapeSizeConverter.h"
#include "model/SubstrateModel.h"
#include "model/core/ModelTypes.h"
#include "scene/ISceneObject.h"
#include "scene/items/CircleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/StickItem.h"
#include "serialization/ProjectSerializer.h"
#include "ui/bindings/ShapeModelBinder.h"
#include "ui/editor/EditorArea.h"
#include "ui/editor/SubstrateItem.h"
#include "ui/utils/ColorUtils.h"

namespace {
constexpr double kDefaultSubstrateWidthPx = 1000.0;
constexpr double kDefaultSubstrateHeightPx = 1000.0;
constexpr int kDefaultSubstrateColorR = 240;
constexpr int kDefaultSubstrateColorG = 240;
constexpr int kDefaultSubstrateColorB = 240;
constexpr int kDefaultSubstrateColorA = 255;
constexpr double kDefaultCircleRadius = 50.0;
}  // namespace

DocumentController::DocumentController(QObject* parent) : QObject(parent) {}

DocumentController::~DocumentController() = default;

void DocumentController::set_document_model(DocumentModel* document) {
  document_model_ = document;
}

void DocumentController::set_shape_binder(ShapeModelBinder* binder) {
  shape_binder_ = binder;
}

void DocumentController::set_editor_area(EditorArea* editor_area) {
  editor_area_ = editor_area;
}

void DocumentController::set_command_manager(CommandManager* command_manager) {
  command_manager_ = command_manager;
}

void DocumentController::new_document() {
  if (document_model_ == nullptr) {
    return;
  }

  // Clear command history when creating new document
  if (command_manager_ != nullptr) {
    command_manager_->clear();
  }

  document_model_->clear_shapes();
  document_model_->clear_materials();
  auto substrate = std::make_shared<SubstrateModel>(
    Size2D{.width = kDefaultSubstrateWidthPx,
           .height = kDefaultSubstrateHeightPx},
    Color{.r = kDefaultSubstrateColorR,
          .g = kDefaultSubstrateColorG,
          .b = kDefaultSubstrateColorB,
          .a = kDefaultSubstrateColorA});
  substrate->set_name("Substrate");
  document_model_->set_substrate(substrate);

  current_file_path_.clear();
  emit file_path_changed(current_file_path_);

  rebuild_scene_from_document();
  emit document_changed();
}

bool DocumentController::save_document(const QString& file_path) {
  if (document_model_ == nullptr) {
    return false;
  }

  sync_document_from_scene();
  const bool success =
    ProjectSerializer::save_to_file(file_path, document_model_);
  if (success) {
    current_file_path_ = file_path;
    emit file_path_changed(current_file_path_);
  }
  return success;
}

bool DocumentController::load_document(const QString& file_path) {
  if (document_model_ == nullptr) {
    return false;
  }

  const bool success =
    ProjectSerializer::load_from_file(file_path, document_model_);
  if (success) {
    current_file_path_ = file_path;
    emit file_path_changed(current_file_path_);
    rebuild_scene_from_document();
    emit document_changed();
  }
  return success;
}

void DocumentController::rebuild_scene_from_document() {
  if (document_model_ == nullptr || editor_area_ == nullptr ||
      shape_binder_ == nullptr) {
    return;
  }

  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  shape_binder_->clear_bindings();
  clear_scene_except_substrate();
  update_substrate_from_model();
  create_shapes_in_scene();
}

void DocumentController::sync_document_from_scene() {
  if (document_model_ == nullptr || editor_area_ == nullptr) {
    return;
  }

  auto substrate = document_model_->substrate();
  if (!substrate) {
    substrate = std::make_shared<SubstrateModel>();
    document_model_->set_substrate(substrate);
  }

  if (auto* substrate_item = editor_area_->substrate_item()) {
    const QSizeF size = substrate_item->size();
    substrate->set_size(Size2D{.width = size.width(), .height = size.height()});
    substrate->set_color(to_model_color(substrate_item->fill_color()));
    substrate->set_name(substrate_item->name().toStdString());
  }
}

auto DocumentController::create_item_for_shape(
  const std::shared_ptr<ShapeModel>& shape) -> ISceneObject* {
  if (shape == nullptr) {
    return nullptr;
  }

  const Size2D size = shape->size();
  switch (shape->type()) {
    case ShapeModel::ShapeType::Rectangle: {
      auto* item = new RectangleItem(QRectF(0, 0, size.width, size.height));
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Ellipse: {
      auto* item = new EllipseItem(QRectF(0, 0, size.width, size.height));
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Circle: {
      const qreal radius = static_cast<qreal>(size.width) / 2.0;
      auto* item = new CircleItem(radius > 0 ? radius : kDefaultCircleRadius);
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Stick: {
      const qreal half_len = static_cast<qreal>(size.width) / 2.0;
      auto* item = new StickItem(QLineF(-half_len, 0.0, half_len, 0.0));
      QPen pen = item->pen();
      pen.setWidthF(ShapeConstants::kStickThickness);
      item->setPen(pen);
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
  }
  return nullptr;
}

void DocumentController::change_shape_type(ISceneObject* old_item,
                                           const QString& new_type) {
  if (old_item == nullptr || editor_area_ == nullptr ||
      shape_binder_ == nullptr) {
    return;
  }

  auto* old_graphics_item = dynamic_cast<QGraphicsItem*>(old_item);
  if (old_graphics_item == nullptr) {
    return;
  }

  // Validate item is still in scene
  if (old_graphics_item->scene() == nullptr) {
    return;
  }

  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  auto shape_model = shape_binder_->model_for(old_item);
  if (shape_model == nullptr) {
    return;
  }

  const ShapeModel::ShapeType target_type = string_to_shape_type(new_type);
  if (shape_model->type() == target_type) {
    return;
  }

  // Get current properties before changing type
  const QPointF old_center = old_graphics_item->sceneBoundingRect().center();
  const qreal rotation = old_graphics_item->rotation();
  const QString item_name = old_item->name();
  const Size2D current_model_size = shape_model->size();
  const ShapeModel::ShapeType current_type = shape_model->type();

  // Convert size and ensure minimum
  const Size2D new_size =
    convert_shape_size(current_type, target_type, current_model_size);
  shape_model->set_type(target_type);
  shape_model->set_size(new_size);

  replace_shape_item(old_item, shape_model, old_center, rotation, item_name);
}

auto DocumentController::string_to_shape_type(const QString& type)
  -> ShapeModel::ShapeType {
  if (type == "circle") {
    return ShapeModel::ShapeType::Circle;
  }
  if (type == "ellipse") {
    return ShapeModel::ShapeType::Ellipse;
  }
  if (type == "stick") {
    return ShapeModel::ShapeType::Stick;
  }
  return ShapeModel::ShapeType::Rectangle;
}

auto DocumentController::convert_shape_size(ShapeModel::ShapeType from,
                                            ShapeModel::ShapeType target_type,
                                            const Size2D& size) -> Size2D {
  return ShapeSizeConverter::convert(from, target_type, size);
}

void DocumentController::replace_shape_item(
  ISceneObject* old_item, const std::shared_ptr<ShapeModel>& model,
  const QPointF& center_position, qreal rotation, const QString& name) {
  if (old_item == nullptr || model == nullptr || editor_area_ == nullptr ||
      shape_binder_ == nullptr) {
    return;
  }

  auto* old_graphics_item = dynamic_cast<QGraphicsItem*>(old_item);
  if (old_graphics_item == nullptr) {
    return;
  }

  // Validate old item is still in scene
  if (old_graphics_item->scene() == nullptr) {
    return;
  }

  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  // Create new item
  auto* new_item = create_item_for_shape(model);
  if (new_item == nullptr) {
    return;
  }

  auto* new_graphics_item = dynamic_cast<QGraphicsItem*>(new_item);
  if (new_graphics_item == nullptr) {
    delete new_item;
    return;
  }

  // Calculate position to preserve center
  const QRectF new_bounding_rect = new_graphics_item->boundingRect();
  if (!new_bounding_rect.isValid()) {
    delete new_item;
    return;
  }

  const QPointF new_position = center_position - new_bounding_rect.center();

  // Unbind old item first
  shape_binder_->unbind_shape(old_item);

  // Remove old item from scene
  scene->removeItem(old_graphics_item);
  delete old_graphics_item;

  // Set up new item
  new_item->set_name(name);
  new_graphics_item->setPos(new_position);
  new_graphics_item->setRotation(rotation);
  scene->addItem(new_graphics_item);

  // Update model position before binding
  model->set_position(Point2D{.x = new_position.x(), .y = new_position.y()});
  model->set_rotation_deg(rotation);

  // Bind new item
  shape_binder_->attach_shape(new_item, model);
  new_graphics_item->update();
}

void DocumentController::clear_scene_except_substrate() {
  if (editor_area_ == nullptr) {
    return;
  }

  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  QList<QGraphicsItem*> to_remove;
  for (QGraphicsItem* item : scene->items()) {
    if (dynamic_cast<SubstrateItem*>(item) == nullptr) {
      to_remove.append(item);
    }
  }

  for (QGraphicsItem* item : to_remove) {
    scene->removeItem(item);
    delete item;
  }
}

void DocumentController::update_substrate_from_model() {
  if (document_model_ == nullptr || editor_area_ == nullptr) {
    return;
  }

  if (auto substrate_model = document_model_->substrate()) {
    if (auto* substrate_item = editor_area_->substrate_item()) {
      substrate_item->set_size(
        QSizeF(substrate_model->size().width, substrate_model->size().height));
      substrate_item->set_fill_color(to_qcolor(substrate_model->color()));
      substrate_item->set_name(QString::fromStdString(substrate_model->name()));
    }
  }
}

void DocumentController::create_shapes_in_scene() {
  if (document_model_ == nullptr || editor_area_ == nullptr ||
      shape_binder_ == nullptr) {
    return;
  }

  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  for (const auto& shape : document_model_->shapes()) {
    if (!shape) {
      continue;
    }

    if (auto* scene_obj = create_item_for_shape(shape)) {
      if (auto* graphics_item = dynamic_cast<QGraphicsItem*>(scene_obj)) {
        scene_obj->set_name(QString::fromStdString(shape->name()));
        graphics_item->setPos(shape->position().x, shape->position().y);
        graphics_item->setRotation(shape->rotation_deg());
        scene->addItem(graphics_item);
        shape_binder_->attach_shape(scene_obj, shape);
      } else {
        delete scene_obj;
      }
    }
  }
}
