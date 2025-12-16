#pragma once

#include <memory>
#include <string>
#include <variant>

#include "commands/Command.h"
#include "model/MaterialModel.h"
#include "model/core/ModelTypes.h"

class DocumentModel;

/**
 * @brief Command to create a new material.
 */
class CreateMaterialCommand : public Command {
 public:
  explicit CreateMaterialCommand(DocumentModel* document, Color color = {},
                        std::string name = {});

  auto execute() -> bool override;
  auto undo() -> bool override;
  [[nodiscard]] auto description() const -> std::string override;

  [[nodiscard]] std::shared_ptr<MaterialModel> created_material() const {
    return created_material_;
  }

 private:
  DocumentModel* document_;
  Color color_;
  std::string name_;
  std::shared_ptr<MaterialModel> created_material_;
  int material_index_{-1};  // Index in document's materials vector
};

/**
 * @brief Command to delete a material.
 */
class DeleteMaterialCommand : public Command {
 public:
  DeleteMaterialCommand(DocumentModel* document,
                        const std::shared_ptr<MaterialModel>& material);

  auto execute() -> bool override;
  auto undo() -> bool override;
  [[nodiscard]] auto description() const -> std::string override;

 private:
  DocumentModel* document_;
  std::shared_ptr<MaterialModel> material_;
  int material_index_{-1};  // Index in document's materials vector
};

/**
 * @brief Command to modify a material property.
 */
class ModifyMaterialPropertyCommand : public Command {
 public:
  enum class Property : std::uint8_t { kName, kColor, kGridType, kGridFrequencyX, kGridFrequencyY };

  ModifyMaterialPropertyCommand(
    std::shared_ptr<MaterialModel> material, Property property,
    const std::variant<std::string, Color, MaterialModel::GridType, double>&
      new_value);

  auto execute() -> bool override;
  auto undo() -> bool override;
  [[nodiscard]] auto description() const -> std::string override;
  auto merge_with(const Command& other) -> bool override;

 private:
  std::shared_ptr<MaterialModel> material_;
  Property property_;
  std::variant<std::string, Color, MaterialModel::GridType, double> new_value_;
  std::variant<std::string, Color, MaterialModel::GridType, double> old_value_;
};
