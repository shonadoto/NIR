#include "commands/MaterialCommands.h"

#include <algorithm>
#include <string>
#include <variant>

#include "model/DocumentModel.h"
#include "model/MaterialModel.h"

// CreateMaterialCommand
CreateMaterialCommand::CreateMaterialCommand(DocumentModel* document,
                                             Color color, std::string name)
    : document_(document), color_(color), name_(std::move(name)) {}

auto CreateMaterialCommand::execute() -> bool {
  if (document_ == nullptr) {
    return false;
  }

  created_material_ = document_->create_material(color_, name_);
  if (created_material_ == nullptr) {
    return false;
  }

  // Find index in document
  const auto& materials = document_->materials();
  const auto material_it =
    std::ranges::find(materials, created_material_);
  if (material_it != materials.end()) {
    material_index_ =
      static_cast<int>(std::distance(materials.begin(), material_it));
  }

  return true;
}

auto CreateMaterialCommand::undo() -> bool {
  if (document_ == nullptr || created_material_ == nullptr) {
    return false;
  }

  document_->remove_material(created_material_);
  created_material_.reset();
  return true;
}

auto CreateMaterialCommand::description() const -> std::string {
  return "Create Material" + (name_.empty() ? "" : " " + name_);
}

// DeleteMaterialCommand
DeleteMaterialCommand::DeleteMaterialCommand(
  DocumentModel* document, const std::shared_ptr<MaterialModel>& material)
    : document_(document), material_(material) {
  // Find index before deletion
  if (document_ != nullptr && material_ != nullptr) {
    const auto& materials = document_->materials();
    const auto material_it =
      std::ranges::find(materials, material_);
    if (material_it != materials.end()) {
      material_index_ =
        static_cast<int>(std::distance(materials.begin(), material_it));
    }
  }
}

auto DeleteMaterialCommand::execute() -> bool {
  if (document_ == nullptr || material_ == nullptr) {
    return false;
  }

  document_->remove_material(material_);
  return true;
}

auto DeleteMaterialCommand::undo() -> bool {
  if (document_ == nullptr || material_ == nullptr) {
    return false;
  }

  // Recreate material
  // Note: DocumentModel doesn't have insert_at, so we'll add at end
  // This is acceptable as order doesn't matter for materials
  auto restored =
    document_->create_material(material_->color(), material_->name());
  if (restored == nullptr) {
    return false;
  }

  // Copy all properties
  restored->set_grid_type(material_->grid_type());
  restored->set_grid_frequency_x(material_->grid_frequency_x());
  restored->set_grid_frequency_y(material_->grid_frequency_y());

  material_ = restored;
  return true;
}

auto DeleteMaterialCommand::description() const -> std::string {
  return "Delete Material";
}

// ModifyMaterialPropertyCommand
ModifyMaterialPropertyCommand::ModifyMaterialPropertyCommand(
  std::shared_ptr<MaterialModel> material, Property property,
  const std::variant<std::string, Color, MaterialModel::GridType, double>&
    new_value)
    : material_(std::move(material)),
      property_(property),
      new_value_(new_value) {
  // Save old value
  if (material_ != nullptr) {
    // NOLINTNEXTLINE(bugprone-branch-clone) - each case calls different method
    switch (property) {
      case Property::kName:
        old_value_ = material_->name();
        break;
      case Property::kColor:
        old_value_ = material_->color();
        break;
      case Property::kGridType:
        old_value_ = material_->grid_type();
        break;
      case Property::kGridFrequencyX:
        old_value_ = material_->grid_frequency_x();
        break;
      case Property::kGridFrequencyY:
        old_value_ = material_->grid_frequency_y();
        break;
    }
  }
}

auto ModifyMaterialPropertyCommand::execute() -> bool {
  if (material_ == nullptr) {
    return false;
  }

  switch (property_) {
    case Property::kName:
      if (std::holds_alternative<std::string>(new_value_)) {
        material_->set_name(std::get<std::string>(new_value_));
      }
      break;
    case Property::kColor:
      if (std::holds_alternative<Color>(new_value_)) {
        material_->set_color(std::get<Color>(new_value_));
      }
      break;
    case Property::kGridType:
      if (std::holds_alternative<MaterialModel::GridType>(new_value_)) {
        material_->set_grid_type(std::get<MaterialModel::GridType>(new_value_));
      }
      break;
    case Property::kGridFrequencyX:
      if (std::holds_alternative<double>(new_value_)) {
        material_->set_grid_frequency_x(std::get<double>(new_value_));
      }
      break;
    case Property::kGridFrequencyY:
      if (std::holds_alternative<double>(new_value_)) {
        material_->set_grid_frequency_y(std::get<double>(new_value_));
      }
      break;
  }

  return true;
}

auto ModifyMaterialPropertyCommand::undo() -> bool {
  if (material_ == nullptr) {
    return false;
  }

  // NOLINTNEXTLINE(bugprone-branch-clone) - each case handles different
  // property type
  switch (property_) {
    case Property::kName:
      if (std::holds_alternative<std::string>(old_value_)) {
        material_->set_name(std::get<std::string>(old_value_));
      }
      break;
    case Property::kColor:
      if (std::holds_alternative<Color>(old_value_)) {
        material_->set_color(std::get<Color>(old_value_));
      }
      break;
    case Property::kGridType:
      if (std::holds_alternative<MaterialModel::GridType>(old_value_)) {
        material_->set_grid_type(std::get<MaterialModel::GridType>(old_value_));
      }
      break;
    case Property::kGridFrequencyX:
      if (std::holds_alternative<double>(old_value_)) {
        material_->set_grid_frequency_x(std::get<double>(old_value_));
      }
      break;
    case Property::kGridFrequencyY:
      if (std::holds_alternative<double>(old_value_)) {
        material_->set_grid_frequency_y(std::get<double>(old_value_));
      }
      break;
  }

  return true;
}

auto ModifyMaterialPropertyCommand::description() const -> std::string {
  switch (property_) {
    case Property::kName:
      return "Rename Material";
    case Property::kColor:
      return "Change Material Color";
    case Property::kGridType:
      return "Change Material Grid Type";
    case Property::kGridFrequencyX:
      return "Change Material Grid Frequency X";
    case Property::kGridFrequencyY:
      return "Change Material Grid Frequency Y";
  }
  return "Modify Material";
}

auto ModifyMaterialPropertyCommand::merge_with(const Command& other) -> bool {
  const auto* other_cmd =
    dynamic_cast<const ModifyMaterialPropertyCommand*>(&other);
  if (other_cmd == nullptr) {
    return false;
  }

  // Only merge if same material and same property
  if (material_ != other_cmd->material_ || property_ != other_cmd->property_) {
    return false;
  }

  // Update new_value_ to other's new_value_ (we're merging forward)
  new_value_ = other_cmd->new_value_;
  return true;
}
