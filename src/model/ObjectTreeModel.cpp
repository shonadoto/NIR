#include "ObjectTreeModel.h"

#include <QIcon>
#include <QVariant>
#include <QGraphicsItem>
#include <QGraphicsScene>

#include "ui/editor/SubstrateItem.h"
#include "scene/ISceneObject.h"

ObjectTreeModel::ObjectTreeModel(QObject *parent)
    : QAbstractItemModel(parent) {}

ObjectTreeModel::~ObjectTreeModel() = default;

void ObjectTreeModel::set_substrate(SubstrateItem *substrate) {
    beginResetModel();
    substrate_ = substrate;
    endResetModel();
}

QModelIndex ObjectTreeModel::index(int row, int column, const QModelIndex &parentIdx) const {
    if (column != 0 || row < 0) return {};
    if (!parentIdx.isValid()) {
        int total = (substrate_ ? 1 : 0) + items_.size();
        if (row < 0 || row >= total) return {};
        if (substrate_) {
            if (row == 0) return createIndex(row, column, substrate_);
            return createIndex(row, column, items_.at(row - 1));
        } else {
            return createIndex(row, column, items_.at(row));
        }
    }
    return {};
}

QModelIndex ObjectTreeModel::parent(const QModelIndex &child) const {
    Q_UNUSED(child);
    return {};
}

int ObjectTreeModel::rowCount(const QModelIndex &parentIdx) const {
    if (!parentIdx.isValid()) {
        return (substrate_ ? 1 : 0) + items_.size();
    }
    return 0;
}

int ObjectTreeModel::columnCount(const QModelIndex &parentIdx) const {
    Q_UNUSED(parentIdx);
    return 1;
}

QVariant ObjectTreeModel::data(const QModelIndex &idx, int role) const {
    if (!idx.isValid() || idx.column() != 0) return {};
    if (role == Qt::DisplayRole) {
        auto *item = static_cast<QGraphicsItem*>(idx.internalPointer());
        if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
            return sceneObj->name();
        }
        return QStringLiteral("Item");
    }
    return {};
}

Qt::ItemFlags ObjectTreeModel::flags(const QModelIndex &idx) const {
    if (!idx.isValid()) return Qt::NoItemFlags;
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
    if (!index.isValid()) return nullptr;
    return static_cast<QGraphicsItem*>(index.internalPointer());
}

QModelIndex ObjectTreeModel::index_from_item(QGraphicsItem *item) const {
    if (item == nullptr) return {};
    if (item == substrate_) {
        return createIndex(0, 0, item);
    }
    return {};
}

void ObjectTreeModel::add_item(QGraphicsItem *item, const QString &name) {
    if (!item) return;
    const int row = rowCount({});
    beginInsertRows({}, row, row);
    items_.append(item);
    names_.insert(item, name);
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
    int first = substrate_ ? 1 : 0;
    int last = first + items_.size() - 1;
    beginRemoveRows({}, first, last);
    items_.clear();
    names_.clear();
    endRemoveRows();
}

bool ObjectTreeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || role != Qt::EditRole) return false;
    auto *item = item_from_index(index);
    if (!item) return false;

    if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
        sceneObj->set_name(value.toString());
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
    }
    return false;
}

bool ObjectTreeModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (count <= 0) return false;
    if (parent.isValid()) return false; // flat list for now
    int total = rowCount({});
    if (row < 0 || row + count > total) return false;
    int first = row;
    int last = row + count - 1;
    // Protect substrate at row 0
    if (substrate_ && first == 0) return false;
    beginRemoveRows(parent, first, last);
    for (int r = last; r >= first; --r) {
        QGraphicsItem *it = substrate_ ? items_.at(r - 1) : items_.at(r);
        names_.remove(it);
    // Defer to item's scene if available
    if (auto *s = it->scene()) { s->removeItem(it); }
        delete it;
        if (substrate_) items_.removeAt(r - 1); else items_.removeAt(r);
    }
    endRemoveRows();
    return true;
}


