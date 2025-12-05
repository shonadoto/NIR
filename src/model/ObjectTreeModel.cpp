#include "ObjectTreeModel.h"

#include <QIcon>
#include <QVariant>
#include <QtGlobal>

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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
QModelIndex ObjectTreeModel::index(int row, int column,
                                   const QModelIndex& parentIdx) const {
  if (column != 0 || row < 0) {
    return {};
  }

  TreeNode* parentNode = node_from_index(parentIdx);

  if (parentNode == root_node()) {
    // Root has two children: Inclusions and Materials
    if (row == 0) {
      return create_index_for_node(inclusions_node(), 0, 0);
    }
    if (row == 1) {
      return create_index_for_node(materials_node(), 1, 0);
    }
    return {};
  }

  if (parentNode == inclusions_node()) {
    const auto& shapes = document_ != nullptr
                           ? document_->shapes()
                           : std::vector<std::shared_ptr<ShapeModel>>{};
    if (document_ == nullptr) {
      return {};
    }
    if (row >= 0 && row < static_cast<int>(shapes.size())) {
      ShapeModel* shapePtr = shapes[row].get();
      TreeNode* itemNode =
        const_cast<ObjectTreeModel*>(this)->shape_nodes_.value(shapePtr,
                                                               nullptr);
      if (itemNode == nullptr) {
        itemNode = new TreeNode(TreeNode::InclusionItem, shapePtr);
        const_cast<ObjectTreeModel*>(this)->shape_nodes_.insert(shapePtr,
                                                                itemNode);
      }
      return create_index_for_node(itemNode, row, column);
    }
    return {};
  }

  if (parentNode == materials_node()) {
    if (document_ == nullptr) {
      return {};
    }
    const auto& materials = document_->materials();
    if (row >= 0 && row < static_cast<int>(materials.size())) {
      MaterialModel* materialPtr = materials[row].get();
      TreeNode* materialNode =
        const_cast<ObjectTreeModel*>(this)->material_nodes_.value(materialPtr,
                                                                  nullptr);
      if (materialNode == nullptr) {
        materialNode = new TreeNode(TreeNode::MaterialItem, materialPtr);
        const_cast<ObjectTreeModel*>(this)->material_nodes_.insert(
          materialPtr, materialNode);
      }
      return create_index_for_node(materialNode, row, column);
    }
    return {};
  }

  return {};
}

QModelIndex ObjectTreeModel::parent(const QModelIndex& child) const {
  if (!child.isValid()) {
    return {};
  }

  TreeNode* childNode = node_from_index(child);

  if (childNode == inclusions_node() || childNode == materials_node()) {
    // These are direct children of root
    return {};
  }

  if (childNode->type == TreeNode::InclusionItem) {
    // Parent is inclusions node
    return create_index_for_node(inclusions_node(), 0, 0);
  }

  if (childNode->type == TreeNode::MaterialItem) {
    // Parent is materials node
    return create_index_for_node(materials_node(), 1, 0);
  }

  return {};
}

int ObjectTreeModel::rowCount(const QModelIndex& parentIdx) const {
  TreeNode* parentNode = node_from_index(parentIdx);

  if (parentNode == root_node()) {
    // Root has 2 children: Inclusions and Materials
    return 2;
  }

  if (parentNode == inclusions_node()) {
    if (document_ != nullptr) {
      return static_cast<int>(document_->shapes().size());
    }
    return 0;
  }

  if (parentNode == materials_node()) {
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
      if (auto* shapePtr = static_cast<ShapeModel*>(node->data)) {
        if (document_ != nullptr) {
          return QString::fromStdString(shapePtr->name());
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
  return QVariant();
}

std::shared_ptr<ShapeModel> ObjectTreeModel::shape_from_index(
  const QModelIndex& index) const {
  if (!index.isValid()) {
    return {};
  }
  TreeNode* node = node_from_index(index);
  if (node != nullptr && node->type == TreeNode::InclusionItem) {
    return document_ != nullptr ? document_->shapes().at(index.row()) : nullptr;
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
      ShapeModel* shapePtr = shapes[i].get();
      TreeNode* node = shape_nodes_.value(shapePtr, nullptr);
      if (node == nullptr) {
        node = new TreeNode(TreeNode::InclusionItem, shapePtr);
        shape_nodes_.insert(shapePtr, node);
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
    auto* materialPtr = static_cast<MaterialModel*>(node->data);
    for (const auto& material : document_->materials()) {
      if (material.get() == materialPtr) {
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
      MaterialModel* materialPtr = materials[i].get();
      TreeNode* node = material_nodes_.value(materialPtr, nullptr);
      if (node == nullptr) {
        node = new TreeNode(TreeNode::MaterialItem, materialPtr);
        material_nodes_.insert(materialPtr, node);
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
  // NOLINTNEXTLINE(readability-delete-null-pointer)
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

  const QString newName = value.toString().trimmed();
  if (newName.isEmpty()) {
    return false;  // Reject empty names
  }

  if (node->type == TreeNode::InclusionItem) {
    if (auto* shapePtr = static_cast<ShapeModel*>(node->data)) {
      if (document_ != nullptr) {
        shapePtr->set_name(newName.toStdString());
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
      }
    }
  }
  if (node->type == TreeNode::MaterialItem) {
    if (auto* material = static_cast<MaterialModel*>(node->data)) {
      material->set_name(newName.toStdString());
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

  TreeNode* parentNode = node_from_index(parent);

  if (parentNode == inclusions_node() && document_ != nullptr) {
    const auto& shapes = document_->shapes();
    if (row < 0 || row + count > static_cast<int>(shapes.size())) {
      return false;
    }
    for (int row_index = 0; row_index < count; ++row_index) {
      auto shape = shapes[row];
      document_->remove_shape(shape);
      // NOLINTNEXTLINE(readability-delete-null-pointer)
      // delete nullptr is safe in C++
      auto* node = shape_nodes_.take(shape.get());
      delete node;
    }
    return true;
  }

  if (parentNode == materials_node() && document_ != nullptr) {
    const auto& materials = document_->materials();
    if (row < 0 || row + count > static_cast<int>(materials.size())) {
      return false;
    }
    for (int row_index = 0; row_index < count; ++row_index) {
      auto material = materials[row];
      document_->remove_material(material);
      // NOLINTNEXTLINE(readability-delete-null-pointer)
      // delete nullptr is safe in C++
      auto* node = material_nodes_.take(material.get());
      delete node;
    }
    beginResetModel();
    endResetModel();
    return true;
  }

  return false;
}
