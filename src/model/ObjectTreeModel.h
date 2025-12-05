#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <memory>

class SubstrateItem;
class MaterialModel;
class DocumentModel;
class ShapeModel;

/**
 * @brief Internal node structure for the tree model
 */
struct TreeNode {
  enum Type { Root, Inclusions, Materials, InclusionItem, MaterialItem };

  Type type;
  void* data{nullptr};  // ShapeModel* for InclusionItem, MaterialModel* for
                        // MaterialItem, nullptr for groups

  TreeNode(Type t, void* d = nullptr) : type(t), data(d) {}
};

class ObjectTreeModel : public QAbstractItemModel {
  Q_OBJECT
public:
  explicit ObjectTreeModel(QObject* parent = nullptr);
  ~ObjectTreeModel() override;

  // Data source
  void set_substrate(SubstrateItem* substrate);
  void set_document(DocumentModel* document);

  // QAbstractItemModel interface
  QModelIndex index(int row, int column,
                    const QModelIndex& parent) const override;
  QModelIndex parent(const QModelIndex& child) const override;
  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& idx, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& idx) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role) override;
  bool removeRows(int row, int count, const QModelIndex& parent) override;

  // Helpers for selection sync
  std::shared_ptr<ShapeModel> shape_from_index(const QModelIndex& index) const;
  QModelIndex index_from_shape(const std::shared_ptr<ShapeModel>& shape) const;
  std::shared_ptr<MaterialModel> material_from_index(
    const QModelIndex& index) const;
  QModelIndex index_from_material(
    const std::shared_ptr<MaterialModel>& material) const;

  // Modification API for inclusions (shapes)
  void clear_items();

  // Modification API for materials
  std::shared_ptr<MaterialModel> create_material(const QString& name = {});
  void remove_material(const std::shared_ptr<MaterialModel>& material);
  void clear_materials();
  DocumentModel* document() const {
    return document_;
  }

private:
  // Helper methods
  TreeNode* node_from_index(const QModelIndex& index) const;
  QModelIndex create_index_for_node(TreeNode* node, int row, int column) const;
  TreeNode* root_node() const {
    return const_cast<TreeNode*>(&root_node_);
  }
  TreeNode* inclusions_node() const {
    return const_cast<TreeNode*>(&inclusions_node_);
  }
  TreeNode* materials_node() const {
    return const_cast<TreeNode*>(&materials_node_);
  }

  // Root nodes (persistent)
  mutable TreeNode root_node_{TreeNode::Root};
  mutable TreeNode inclusions_node_{TreeNode::Inclusions};
  mutable TreeNode materials_node_{TreeNode::Materials};

  SubstrateItem* substrate_{nullptr};
  QString substrate_name_{"Substrate"};
  mutable QHash<MaterialModel*, TreeNode*>
    material_nodes_;  // Map materials to their tree nodes
  mutable QHash<ShapeModel*, TreeNode*> shape_nodes_;
  DocumentModel* document_{nullptr};
  int document_connection_{0};
};
