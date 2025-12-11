#pragma once

#include <memory>
#include <string>

#include "commands/Command.h"
#include "model/ShapeModel.h"
#include "model/core/ModelTypes.h"

class DocumentModel;
class ShapeModelBinder;
class EditorArea;
class ISceneObject;
class MaterialModel;

/**
 * @brief Command to create a new shape.
 */
class CreateShapeCommand : public Command {
 public:
  CreateShapeCommand(DocumentModel* document, ShapeModelBinder* binder,
                     EditorArea* editor_area, ShapeModel::ShapeType type,
                     std::string name = {});

  auto execute() -> bool override;
  auto undo() -> bool override;
  [[nodiscard]] auto description() const -> std::string override;

 private:
  DocumentModel* document_;
  ShapeModelBinder* binder_;
  EditorArea* editor_area_;
  ShapeModel::ShapeType type_;
  std::string name_;
  std::shared_ptr<ShapeModel> created_shape_;
  ISceneObject* created_item_{nullptr};
};

/**
 * @brief Command to delete a shape.
 */
class DeleteShapeCommand : public Command {
 public:
  DeleteShapeCommand(DocumentModel* document, ShapeModelBinder* binder,
                     EditorArea* editor_area,
                     const std::shared_ptr<ShapeModel>& shape);

  auto execute() -> bool override;
  auto undo() -> bool override;
  [[nodiscard]] auto description() const -> std::string override;

 private:
  DocumentModel* document_;
  ShapeModelBinder* binder_;
  EditorArea* editor_area_;
  std::shared_ptr<ShapeModel> shape_;
  ISceneObject* item_{nullptr};
  Point2D saved_position_;
  Size2D saved_size_;
  double saved_rotation_{0.0};
  std::string saved_name_;
  ShapeModel::ShapeType saved_type_;
  int shape_index_{-1};  // Index in document's shapes vector
};

/**
 * @brief Command to modify a shape property.
 */
class ModifyShapePropertyCommand : public Command {
 public:
  enum class Property { Name, Position, Size, Rotation, Color, Material };

  ModifyShapePropertyCommand(
    const std::shared_ptr<ShapeModel>& shape, Property property,
    const std::variant<std::string, Point2D, Size2D, double, Color,
                       std::shared_ptr<MaterialModel>>& new_value);

  auto execute() -> bool override;
  auto undo() -> bool override;
  [[nodiscard]] auto description() const -> std::string override;
  auto merge_with(const Command& other) -> bool override;

 private:
  std::shared_ptr<ShapeModel> shape_;
  Property property_;
  std::variant<std::string, Point2D, Size2D, double, Color,
               std::shared_ptr<MaterialModel>>
    new_value_;
  std::variant<std::string, Point2D, Size2D, double, Color,
               std::shared_ptr<MaterialModel>>
    old_value_;
};

/**
 * @brief Command to change shape type.
 */
class ChangeShapeTypeCommand : public Command {
 public:
  ChangeShapeTypeCommand(DocumentModel* document, ShapeModelBinder* binder,
                         EditorArea* editor_area,
                         std::shared_ptr<ShapeModel> shape,
                         ShapeModel::ShapeType new_type);

  auto execute() -> bool override;
  auto undo() -> bool override;
  [[nodiscard]] auto description() const -> std::string override;

 private:
  DocumentModel* document_;
  ShapeModelBinder* binder_;
  EditorArea* editor_area_;
  std::shared_ptr<ShapeModel> shape_;
  ShapeModel::ShapeType new_type_;
  ShapeModel::ShapeType old_type_;
  ISceneObject* old_item_{nullptr};
  ISceneObject* new_item_{nullptr};
};
