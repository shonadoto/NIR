#include "ObjectTreeModel.h"

#include <QIcon>
#include <QVariant>
#include <QGraphicsItem>
#include <QGraphicsScene>

#include "ui/editor/SubstrateItem.h"
#include "scene/ISceneObject.h"
#include "model/MaterialPreset.h"

ObjectTreeModel::ObjectTreeModel(QObject *parent)
    : QAbstractItemModel(parent) {
  // Initialize root node structure
  root_node_.data = nullptr;
  inclusions_node_.data = nullptr;
  materials_node_.data = nullptr;
}

ObjectTreeModel::~ObjectTreeModel() {
  // Clean up presets
  for (auto *preset : presets_) {
    delete preset;
  }
  presets_.clear();
}

void ObjectTreeModel::set_substrate(SubstrateItem *substrate) {
  beginResetModel();
  substrate_ = substrate;
  endResetModel();
}

TreeNode* ObjectTreeModel::node_from_index(const QModelIndex &index) const {
  if (!index.isValid()) {
    return root_node();
  }
  return static_cast<TreeNode*>(index.internalPointer());
}

QModelIndex ObjectTreeModel::create_index_for_node(TreeNode *node, int row, int column) const {
  return createIndex(row, column, node);
}

QModelIndex ObjectTreeModel::index(int row, int column, const QModelIndex &parentIdx) const {
  if (column != 0 || row < 0) {
    return {};
  }

  TreeNode *parentNode = node_from_index(parentIdx);

  if (parentNode == root_node()) {
    // Root has two children: Inclusions and Materials
    if (row == 0) {
      return create_index_for_node(inclusions_node(), 0, 0);
    } else if (row == 1) {
      return create_index_for_node(materials_node(), 1, 0);
    }
    return {};
  }

  if (parentNode == inclusions_node()) {
    // Inclusions node contains all inclusion items
    if (row >= 0 && row < items_.size()) {
      TreeNode *itemNode = const_cast<ObjectTreeModel*>(this)->item_to_node_.value(items_[row], nullptr);
      if (!itemNode) {
        // Create node for this item
        itemNode = new TreeNode(TreeNode::InclusionItem, items_[row]);
        const_cast<ObjectTreeModel*>(this)->item_to_node_.insert(items_[row], itemNode);
      }
      return create_index_for_node(itemNode, row, 0);
    }
    return {};
  }

  if (parentNode == materials_node()) {
    // Materials node contains all material presets
    if (row >= 0 && row < presets_.size()) {
      TreeNode *presetNode = const_cast<ObjectTreeModel*>(this)->preset_to_node_.value(presets_[row], nullptr);
      if (!presetNode) {
        // Create node for this preset
        presetNode = new TreeNode(TreeNode::MaterialPresetItem, presets_[row]);
        const_cast<ObjectTreeModel*>(this)->preset_to_node_.insert(presets_[row], presetNode);
      }
      return create_index_for_node(presetNode, row, 0);
    }
    return {};
  }

  return {};
}

QModelIndex ObjectTreeModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) {
    return {};
  }

  TreeNode *childNode = node_from_index(child);

  if (childNode == inclusions_node() || childNode == materials_node()) {
    // These are direct children of root
    return {};
  }

  if (childNode->type == TreeNode::InclusionItem) {
    // Parent is inclusions node
    return create_index_for_node(inclusions_node(), 0, 0);
  }

  if (childNode->type == TreeNode::MaterialPresetItem) {
    // Parent is materials node
    return create_index_for_node(materials_node(), 1, 0);
  }

  return {};
}

int ObjectTreeModel::rowCount(const QModelIndex &parentIdx) const {
  TreeNode *parentNode = node_from_index(parentIdx);

  if (parentNode == root_node()) {
    // Root has 2 children: Inclusions and Materials
    return 2;
  }

  if (parentNode == inclusions_node()) {
    // Inclusions node contains all inclusion items
    return items_.size();
  }

  if (parentNode == materials_node()) {
    // Materials node contains all presets
    return presets_.size();
  }

  // Leaf nodes have no children
  return 0;
}

int ObjectTreeModel::columnCount(const QModelIndex &parentIdx) const {
  Q_UNUSED(parentIdx);
  return 1;
}

QVariant ObjectTreeModel::data(const QModelIndex &idx, int role) const {
  if (!idx.isValid() || idx.column() != 0) {
    return {};
  }

  TreeNode *node = node_from_index(idx);

  if (role == Qt::DisplayRole) {
    if (node == inclusions_node()) {
      return QStringLiteral("Inclusions");
    }
    if (node == materials_node()) {
      return QStringLiteral("Materials");
    }
    if (node->type == TreeNode::InclusionItem) {
      auto *item = static_cast<QGraphicsItem*>(node->data);
      if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
        return sceneObj->name();
      }
      return QStringLiteral("Item");
    }
    if (node->type == TreeNode::MaterialPresetItem) {
      auto *preset = static_cast<MaterialPreset*>(node->data);
      return preset->name();
    }
  }

  return {};
}

Qt::ItemFlags ObjectTreeModel::flags(const QModelIndex &idx) const {
  if (!idx.isValid()) {
    return Qt::NoItemFlags;
  }

  TreeNode *node = node_from_index(idx);

  // Group nodes (Inclusions, Materials) are not editable
  if (node == inclusions_node() || node == materials_node()) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  // Items and presets are editable
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant ObjectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    Q_UNUSED(section);
    return QString(); // no visible header text
  }
  return QVariant();
}

QGraphicsItem* ObjectTreeModel::item_from_index(const QModelIndex &index) const {
  if (!index.isValid()) {
    return nullptr;
  }
  TreeNode *node = node_from_index(index);
  if (node && node->type == TreeNode::InclusionItem) {
    return static_cast<QGraphicsItem*>(node->data);
  }
  return nullptr;
}

QModelIndex ObjectTreeModel::index_from_item(QGraphicsItem *item) const {
  if (item == nullptr) {
    return {};
  }
  TreeNode *node = item_to_node_.value(item, nullptr);
  if (node) {
    int row = items_.indexOf(item);
    if (row >= 0) {
      return create_index_for_node(node, row, 0);
    }
  }
  return {};
}

MaterialPreset* ObjectTreeModel::preset_from_index(const QModelIndex &index) const {
  if (!index.isValid()) {
    return nullptr;
  }
  TreeNode *node = node_from_index(index);
  if (node && node->type == TreeNode::MaterialPresetItem) {
    return static_cast<MaterialPreset*>(node->data);
  }
  return nullptr;
}

QModelIndex ObjectTreeModel::index_from_preset(MaterialPreset *preset) const {
  if (preset == nullptr) {
    return {};
  }
  TreeNode *node = preset_to_node_.value(preset, nullptr);
  if (node) {
    int row = presets_.indexOf(preset);
    if (row >= 0) {
      return create_index_for_node(node, row, 0);
    }
  }
  return {};
}

void ObjectTreeModel::add_item(QGraphicsItem *item, const QString &name) {
  if (!item) {
    return;
  }
  const int row = items_.size();
  QModelIndex parentIdx = create_index_for_node(inclusions_node(), 0, 0);
  beginInsertRows(parentIdx, row, row);
  items_.append(item);
  names_.insert(item, name);
  // Create node for this item
  TreeNode *itemNode = new TreeNode(TreeNode::InclusionItem, item);
  item_to_node_.insert(item, itemNode);
  endInsertRows();
}

QString ObjectTreeModel::get_item_name(QGraphicsItem *item) const {
  if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
    return sceneObj->name();
  }
  return QStringLiteral("Item");
}

void ObjectTreeModel::clear_items() {
  if (items_.isEmpty()) {
    return;
  }
  QModelIndex parentIdx = create_index_for_node(inclusions_node(), 0, 0);
  int first = 0;
  int last = items_.size() - 1;
  beginRemoveRows(parentIdx, first, last);
  // Clean up nodes
  for (auto *item : items_) {
    TreeNode *node = item_to_node_.take(item);
    delete node;
  }
  items_.clear();
  names_.clear();
  endRemoveRows();
}

void ObjectTreeModel::add_preset(MaterialPreset *preset) {
  if (!preset) {
    return;
  }
  const int row = presets_.size();
  QModelIndex parentIdx = create_index_for_node(materials_node(), 1, 0);
  beginInsertRows(parentIdx, row, row);
  presets_.append(preset);
  // Create node for this preset
  TreeNode *presetNode = new TreeNode(TreeNode::MaterialPresetItem, preset);
  preset_to_node_.insert(preset, presetNode);
  endInsertRows();
}

void ObjectTreeModel::remove_preset(MaterialPreset *preset) {
  if (!preset) {
    return;
  }
  int row = presets_.indexOf(preset);
  if (row < 0) {
    return;
  }
  QModelIndex parentIdx = create_index_for_node(materials_node(), 1, 0);
  beginRemoveRows(parentIdx, row, row);
  presets_.removeAt(row);
  TreeNode *node = preset_to_node_.take(preset);
  delete node;
  endRemoveRows();
}

QVector<MaterialPreset*> ObjectTreeModel::get_presets() const {
  return presets_;
}

void ObjectTreeModel::clear_presets() {
  if (presets_.isEmpty()) {
    return;
  }
  QModelIndex parentIdx = create_index_for_node(materials_node(), 1, 0);
  int first = 0;
  int last = presets_.size() - 1;
  beginRemoveRows(parentIdx, first, last);
  // Clean up nodes
  for (auto *preset : presets_) {
    TreeNode *node = preset_to_node_.take(preset);
    delete node;
  }
  presets_.clear();
  endRemoveRows();
}

bool ObjectTreeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || role != Qt::EditRole) {
    return false;
  }

  TreeNode *node = node_from_index(index);

  QString newName = value.toString().trimmed();
  if (newName.isEmpty()) {
    return false; // Reject empty names
  }

  if (node->type == TreeNode::InclusionItem) {
    auto *item = static_cast<QGraphicsItem*>(node->data);
    if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
      sceneObj->set_name(newName);
      emit dataChanged(index, index, {Qt::DisplayRole});
      return true;
    }
  } else if (node->type == TreeNode::MaterialPresetItem) {
    auto *preset = static_cast<MaterialPreset*>(node->data);
    preset->set_name(newName);
    emit dataChanged(index, index, {Qt::DisplayRole});
    return true;
  }

  return false;
}

bool ObjectTreeModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (count <= 0) {
    return false;
  }

  TreeNode *parentNode = node_from_index(parent);

  if (parentNode == inclusions_node()) {
    // Removing inclusion items
    int total = items_.size();
    if (row < 0 || row + count > total) {
      return false;
    }
    int first = row;
    int last = row + count - 1;
    beginRemoveRows(parent, first, last);
    for (int r = last; r >= first; --r) {
      QGraphicsItem *it = items_.at(r);
      names_.remove(it);
      // Clean up node
      TreeNode *node = item_to_node_.take(it);
      delete node;
      // Defer to item's scene if available
      if (auto *s = it->scene()) {
        s->removeItem(it);
      }
      delete it;
      items_.removeAt(r);
    }
    endRemoveRows();
    return true;
  }

  if (parentNode == materials_node()) {
    // Removing material presets
    int total = presets_.size();
    if (row < 0 || row + count > total) {
      return false;
    }
    int first = row;
    int last = row + count - 1;
    beginRemoveRows(parent, first, last);
    for (int r = last; r >= first; --r) {
      MaterialPreset *preset = presets_.at(r);
      // Clean up node
      TreeNode *node = preset_to_node_.take(preset);
      delete node;
      delete preset;
      presets_.removeAt(r);
    }
    endRemoveRows();
    return true;
  }

  return false;
}
