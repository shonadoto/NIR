#pragma once

#include <QAbstractItemModel>
#include <QGraphicsItem>

class SubstrateItem;

class ObjectTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum class NodeType { Root, Substrate };

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

    // Modification API
    void add_item(QGraphicsItem *item, const QString &name);
    QString get_item_name(QGraphicsItem *item) const;
    void clear_items();

private:
    SubstrateItem *substrate_ {nullptr};
    QString substrate_name_ {"Substrate"};
    QVector<QGraphicsItem*> items_; // future shapes
    QHash<QGraphicsItem*, QString> names_;
    QGraphicsScene *scene_ {nullptr};
};


