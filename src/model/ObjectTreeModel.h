#pragma once

#include <QAbstractItemModel>
#include <QGraphicsItem>
#include <QVector>
#include <QHash>

class SubstrateItem;
class MaterialPreset;

/**
 * @brief Internal node structure for the tree model
 */
struct TreeNode {
  enum Type { Root, Inclusions, Materials, InclusionItem, MaterialPresetItem };

  Type type;
  void *data{nullptr}; // QGraphicsItem* for InclusionItem, MaterialPreset* for MaterialPresetItem, nullptr for groups

  TreeNode(Type t, void *d = nullptr) : type(t), data(d) {}
};

class ObjectTreeModel : public QAbstractItemModel {
  Q_OBJECT
public:
  explicit ObjectTreeModel(QObject *parent = nullptr);
  ~ObjectTreeModel() override;

  // Data source
  void set_substrate(SubstrateItem *substrate);

  // QAbstractItemModel interface
  QModelIndex index(int row, int column, const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  bool removeRows(int row, int count, const QModelIndex &parent) override;

  // Helpers for selection sync
  QGraphicsItem* item_from_index(const QModelIndex &index) const;
  QModelIndex index_from_item(QGraphicsItem *item) const;
  MaterialPreset* preset_from_index(const QModelIndex &index) const;
  QModelIndex index_from_preset(MaterialPreset *preset) const;

  // Modification API for inclusions (shapes)
  void add_item(QGraphicsItem *item, const QString &name);
  QString get_item_name(QGraphicsItem *item) const;
  void clear_items();

  // Modification API for material presets
  void add_preset(MaterialPreset *preset);
  void remove_preset(MaterialPreset *preset);
  QVector<MaterialPreset*> get_presets() const;
  void clear_presets();

private:
  // Helper methods
  TreeNode* node_from_index(const QModelIndex &index) const;
  QModelIndex create_index_for_node(TreeNode *node, int row, int column) const;
  TreeNode* root_node() const { return const_cast<TreeNode*>(&root_node_); }
  TreeNode* inclusions_node() const { return const_cast<TreeNode*>(&inclusions_node_); }
  TreeNode* materials_node() const { return const_cast<TreeNode*>(&materials_node_); }

  // Root nodes (persistent)
  mutable TreeNode root_node_{TreeNode::Root};
  mutable TreeNode inclusions_node_{TreeNode::Inclusions};
  mutable TreeNode materials_node_{TreeNode::Materials};

  SubstrateItem *substrate_{nullptr};
  QString substrate_name_{"Substrate"};
  QVector<QGraphicsItem*> items_; // Inclusion shapes (excluding substrate)
  QHash<QGraphicsItem*, QString> names_;
  QVector<MaterialPreset*> presets_; // Material presets
  QHash<QGraphicsItem*, TreeNode*> item_to_node_; // Map items to their tree nodes
  QHash<MaterialPreset*, TreeNode*> preset_to_node_; // Map presets to their tree nodes
};


