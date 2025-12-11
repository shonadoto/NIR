#include "ObjectTreeModel.h"

#include <QIcon>
#include <QVariant>
#include <QtGlobal>
#include <vector>

#include "model/DocumentModel.h"
#include "model/MaterialModel.h"
#include "model/ShapeModel.h"
#include "ui/editor/SubstrateItem.h"

ObjectTreeModel::ObjectTreeModel(QObject* parent) : QAbstractItemModel(parent) {
  // Initialize root node structure
  root_node_.data = nullptr;
  inclusions_node_.data = nullptr;
  materials_node_.data = nullptr;
}

ObjectTreeModel::~ObjectTreeModel() {
  if (document_ != nullptr && document_connection_ != 0) {
    document_->on_changed().disconnect(document_connection_);
    document_connection_ = 0;
  }

  qDeleteAll(shape_nodes_);
  shape_nodes_.clear();
  qDeleteAll(material_nodes_);
  material_nodes_.clear();
}

void ObjectTreeModel::set_substrate(SubstrateItem* substrate) {
  beginResetModel();
  substrate_ = substrate;
  endResetModel();
}

void ObjectTreeModel::set_document(DocumentModel* document) {
  if (document_ == document) {
    return;
  }
  if (document_ != nullptr && document_connection_ != 0) {
    document_->on_changed().disconnect(document_connection_);
    document_connection_ = 0;
  }
  document_ = document;
  if (document_ != nullptr) {
    document_connection_ =
      document_->on_changed().connect([this](const ModelChange& change) {
        if (change.type != ModelChange::Type::Custom) {
          return;
        }
        qDeleteAll(shape_nodes_);
        shape_nodes_.clear();
        qDeleteAll(material_nodes_);
        material_nodes_.clear();
        beginResetModel();
        endResetModel();
      });
  }
  beginResetModel();
  endResetModel();
}

TreeNode* ObjectTreeModel::node_from_index(const QModelIndex& index) const {
  if (!index.isValid()) {
    return root_node();
  }
  return static_cast<TreeNode*>(index.internalPointer());
}

QModelIndex ObjectTreeModel::create_index_for_node(TreeNode* node, int row,
                                                   int column) const {
  return createIndex(row, column, node);
}

QModelIndex ObjectTreeModel::index(int row, int column,
                                   const QModelIndex& parentIdx) const {
  if (column != 0 || row < 0) {
    return {};
  }

  TreeNode* parent_node = node_from_index(parentIdx);

  if (parent_node == root_node()) {
    // Root has two children: Inclusions and Materials
    if (row == 0) {
      return create_index_for_node(inclusions_node(), 0, 0);
    }
    if (row == 1) {
      return create_index_for_node(materials_node(), 1, 0);
    }
    return {};
  }

  if (parent_node == inclusions_node()) {
    const auto& shapes = document_ != nullptr
                           ? document_->shapes()
                           : std::vector<std::shared_ptr<ShapeModel>>{};
    if (document_ == nullptr) {
      return {};
    }
    if (row >= 0 && row < static_cast<int>(shapes.size())) {
      ShapeModel* shape_ptr = shapes[row].get();
      TreeNode* item_node =
        const_cast<ObjectTreeModel*>(this)->shape_nodes_.value(shape_ptr,
                                                               nullptr);
      if (item_node == nullptr) {
        item_node = new TreeNode(TreeNode::InclusionItem, shape_ptr);
        const_cast<ObjectTreeModel*>(this)->shape_nodes_.insert(shape_ptr,
                                                                item_node);
      }
      return create_index_for_node(item_node, row, column);
    }
    return {};
  }

  if (parent_node == materials_node()) {
    if (document_ == nullptr) {
      return {};
    }
    const auto& materials = document_->materials();
    if (row >= 0 && row < static_cast<int>(materials.size())) {
      MaterialModel* material_ptr = materials[row].get();
      TreeNode* material_node =
        const_cast<ObjectTreeModel*>(this)->material_nodes_.value(material_ptr,
                                                                  nullptr);
      if (material_node == nullptr) {
        material_node = new TreeNode(TreeNode::MaterialItem, material_ptr);
        const_cast<ObjectTreeModel*>(this)->material_nodes_.insert(
          material_ptr, material_node);
      }
      return create_index_for_node(material_node, row, column);
    }
    return {};
  }

  return {};
}

QModelIndex ObjectTreeModel::parent(const QModelIndex& child) const {
  if (!child.isValid()) {
    return {};
  }

  TreeNode* child_node = node_from_index(child);

  if (child_node == inclusions_node() || child_node == materials_node()) {
    // These are direct children of root
    return {};
  }

  if (child_node->type == TreeNode::InclusionItem) {
    // Parent is inclusions node
    return create_index_for_node(inclusions_node(), 0, 0);
  }

  if (child_node->type == TreeNode::MaterialItem) {
    // Parent is materials node
    return create_index_for_node(materials_node(), 1, 0);
  }

  return {};
}

int ObjectTreeModel::rowCount(const QModelIndex& parentIdx) const {
  TreeNode* parent_node = node_from_index(parentIdx);

  if (parent_node == root_node()) {
    // Root has 2 children: Inclusions and Materials
    return 2;
  }

  if (parent_node == inclusions_node()) {
    if (document_ != nullptr) {
      return static_cast<int>(document_->shapes().size());
    }
    return 0;
  }

  if (parent_node == materials_node()) {
    if (document_ != nullptr) {
      return static_cast<int>(document_->materials().size());
    }
    return 0;
  }

  // Leaf nodes have no children
  return 0;
}

int ObjectTreeModel::columnCount(const QModelIndex& parentIdx) const {
  Q_UNUSED(parentIdx);
  return 1;
}

QVariant ObjectTreeModel::data(const QModelIndex& idx, int role) const {
  if (!idx.isValid() || idx.column() != 0) {
    return {};
  }

  TreeNode* node = node_from_index(idx);

  if (role == Qt::DisplayRole) {
    if (node == inclusions_node()) {
      return QStringLiteral("Inclusions");
    }
    if (node == materials_node()) {
      return QStringLiteral("Materials");
    }
    if (node->type == TreeNode::InclusionItem) {
      if (auto* shape_ptr = static_cast<ShapeModel*>(node->data)) {
        if (document_ != nullptr) {
          return QString::fromStdString(shape_ptr->name());
        }
      }
      return QStringLiteral("Item");
    }
    if (node->type == TreeNode::MaterialItem) {
      if (auto* material = static_cast<MaterialModel*>(node->data)) {
        return QString::fromStdString(material->name());
      }
      return QStringLiteral("Material");
    }
  }

  return {};
}

Qt::ItemFlags ObjectTreeModel::flags(const QModelIndex& idx) const {
  if (!idx.isValid()) {
    return Qt::NoItemFlags;
  }

  TreeNode* node = node_from_index(idx);

  // Group nodes (Inclusions, Materials) are not editable
  if (node == inclusions_node() || node == materials_node()) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  // Items and presets are editable
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant ObjectTreeModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    Q_UNUSED(section);
    return QString();  // no visible header text
  }
  return {};
}

std::shared_ptr<ShapeModel> ObjectTreeModel::shape_from_index(
  const QModelIndex& index) const {
  if (!index.isValid() || document_ == nullptr) {
    return {};
  }
  TreeNode* node = node_from_index(index);
  if (node != nullptr && node->type == TreeNode::InclusionItem) {
    const auto& shapes = document_->shapes();
    const int row = index.row();
    if (row >= 0 && row < static_cast<int>(shapes.size())) {
      return shapes[row];
    }
  }
  return {};
}

QModelIndex ObjectTreeModel::index_from_shape(
  const std::shared_ptr<ShapeModel>& shape) const {
  if (document_ == nullptr || shape == nullptr) {
    return {};
  }
  const auto& shapes = document_->shapes();
  for (int i = 0; i < static_cast<int>(shapes.size()); ++i) {
    if (shapes[i] == shape) {
      ShapeModel* shape_ptr = shapes[i].get();
      TreeNode* node = shape_nodes_.value(shape_ptr, nullptr);
      if (node == nullptr) {
        node = new TreeNode(TreeNode::InclusionItem, shape_ptr);
        shape_nodes_.insert(shape_ptr, node);
      }
      return create_index_for_node(node, i, 0);
    }
  }
  return {};
}

std::shared_ptr<MaterialModel> ObjectTreeModel::material_from_index(
  const QModelIndex& index) const {
  if (!index.isValid() || document_ == nullptr) {
    return nullptr;
  }
  TreeNode* node = node_from_index(index);
  if (node != nullptr && node->type == TreeNode::MaterialItem) {
    auto* material_ptr = static_cast<MaterialModel*>(node->data);
    for (const auto& material : document_->materials()) {
      if (material.get() == material_ptr) {
        return material;
      }
    }
  }
  return nullptr;
}

QModelIndex ObjectTreeModel::index_from_material(
  const std::shared_ptr<MaterialModel>& material) const {
  if (material == nullptr || document_ == nullptr) {
    return {};
  }
  const auto& materials = document_->materials();
  for (int i = 0; i < static_cast<int>(materials.size()); ++i) {
    if (materials[i] == material) {
      MaterialModel* material_ptr = materials[i].get();
      TreeNode* node = material_nodes_.value(material_ptr, nullptr);
      if (node == nullptr) {
        node = new TreeNode(TreeNode::MaterialItem, material_ptr);
        material_nodes_.insert(material_ptr, node);
      }
      return create_index_for_node(node, i, 0);
    }
  }
  return {};
}

void ObjectTreeModel::clear_items() {
  qDeleteAll(shape_nodes_);
  shape_nodes_.clear();
  beginResetModel();
  endResetModel();
}

std::shared_ptr<MaterialModel> ObjectTreeModel::create_material(
  const QString& name) {
  if (document_ == nullptr) {
    return nullptr;
  }
  return document_->create_material({}, name.toStdString());
}

void ObjectTreeModel::remove_material(
  const std::shared_ptr<MaterialModel>& material) {
  if (document_ == nullptr || material == nullptr) {
    return;
  }
  document_->remove_material(material);
  auto* node = material_nodes_.take(material.get());
  // delete nullptr is safe in C++
  delete node;
}

void ObjectTreeModel::clear_materials() {
  if (document_ == nullptr) {
    return;
  }
  document_->clear_materials();
  qDeleteAll(material_nodes_);
  material_nodes_.clear();
}

bool ObjectTreeModel::setData(const QModelIndex& index, const QVariant& value,
                              int role) {
  if (!index.isValid() || role != Qt::EditRole) {
    return false;
  }

  TreeNode* node = node_from_index(index);

  const QString new_name = value.toString().trimmed();
  if (new_name.isEmpty()) {
    return false;  // Reject empty names
  }

  if (node->type == TreeNode::InclusionItem) {
    if (auto* shape_ptr = static_cast<ShapeModel*>(node->data)) {
      if (document_ != nullptr) {
        shape_ptr->set_name(new_name.toStdString());
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
      }
    }
  }
  if (node->type == TreeNode::MaterialItem) {
    if (auto* material = static_cast<MaterialModel*>(node->data)) {
      material->set_name(new_name.toStdString());
      emit dataChanged(index, index, {Qt::DisplayRole});
      return true;
    }
  }

  return false;
}

bool ObjectTreeModel::removeRows(int row, int count,
                                 const QModelIndex& parent) {
  if (count <= 0) {
    return false;
  }

  TreeNode* parent_node = node_from_index(parent);

  if (parent_node == inclusions_node() && document_ != nullptr) {
    const auto& shapes = document_->shapes();
    if (row < 0 || row + count > static_cast<int>(shapes.size())) {
      return false;
    }
    // Collect shapes to remove before deletion (to avoid index issues)
    std::vector<std::shared_ptr<ShapeModel>> shapes_to_remove;
    shapes_to_remove.reserve(count);
    for (int i = 0; i < count; ++i) {
      const int current_row = row + i;
      if (current_row >= 0 && current_row < static_cast<int>(shapes.size())) {
        shapes_to_remove.push_back(shapes[current_row]);
      }
    }
    if (shapes_to_remove.empty()) {
      return false;
    }
    // Remove from end to avoid index shifting issues
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = static_cast<int>(shapes_to_remove.size()) - 1; i >= 0; --i) {
      document_->remove_shape(shapes_to_remove[i]);
      // delete nullptr is safe in C++
      auto* node = shape_nodes_.take(shapes_to_remove[i].get());
      delete node;
    }
    endRemoveRows();
    return true;
  }

  if (parent_node == materials_node() && document_ != nullptr) {
    const auto& materials = document_->materials();
    if (row < 0 || row + count > static_cast<int>(materials.size())) {
      return false;
    }
    // Collect materials to remove before deletion (to avoid index issues)
    std::vector<std::shared_ptr<MaterialModel>> materials_to_remove;
    materials_to_remove.reserve(count);
    for (int i = 0; i < count; ++i) {
      const int current_row = row + i;
      if (current_row >= 0 &&
          current_row < static_cast<int>(materials.size())) {
        materials_to_remove.push_back(materials[current_row]);
      }
    }
    if (materials_to_remove.empty()) {
      return false;
    }
    // Remove from end to avoid index shifting issues
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = static_cast<int>(materials_to_remove.size()) - 1; i >= 0;
         --i) {
      document_->remove_material(materials_to_remove[i]);
      // delete nullptr is safe in C++
      auto* node = material_nodes_.take(materials_to_remove[i].get());
      delete node;
    }
    endRemoveRows();
    return true;
  }

  return false;
}
