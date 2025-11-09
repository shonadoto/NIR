#include "ObjectsBar.h"

#include <QEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QTreeView>
#include <QVBoxLayout>
#include <algorithm>

namespace {

constexpr int kMinObjectsBarWidthPx = 220;
constexpr int kDefaultObjectsBarWidthPx = 280;

} // namespace

ObjectsBar::ObjectsBar(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  tree_view_ = new QTreeView(this);
  layout->addWidget(tree_view_);
  tree_view_->setHeaderHidden(true);
  tree_view_->setEditTriggers(QAbstractItemView::EditKeyPressed |
                              QAbstractItemView::SelectedClicked |
                              QAbstractItemView::DoubleClicked);

  preferred_width_ = kDefaultObjectsBarWidthPx;
  last_visible_width_ = 0;
  setMinimumWidth(kMinObjectsBarWidthPx);
  setFixedWidth(preferred_width_);
}

ObjectsBar::~ObjectsBar() = default;

void ObjectsBar::setPreferredWidth(int w) {
  preferred_width_ = w;
  setFixedWidth(preferred_width_);
}

void ObjectsBar::hideEvent(QHideEvent *event) {
  last_visible_width_ = width();
  QWidget::hideEvent(event);
}

void ObjectsBar::showEvent(QShowEvent *event) {
  const int target =
      last_visible_width_ > 0 ? last_visible_width_ : preferred_width_;
  setFixedWidth(target);
  QWidget::showEvent(event);
}

void ObjectsBar::set_model(QAbstractItemModel *model) {
  tree_view_->setModel(model);
  tree_view_->installEventFilter(this);
}

bool ObjectsBar::eventFilter(QObject *obj, QEvent *event) {
  if (obj == tree_view_) {
    if (event->type() == QEvent::KeyPress) {
      auto *ke = static_cast<QKeyEvent *>(event);
      if (ke->isAutoRepeat()) {
        return false; // ignore auto-repeat to prevent deleting multiple items
                      // unintentionally
      }
      if (ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace) {
        if (auto *m = tree_view_->model()) {
          if (auto *sel = tree_view_->selectionModel()) {
            // Delete all selected rows (column 0) from bottom to top
            QList<QModelIndex> rows = sel->selectedRows(0);
            std::sort(rows.begin(), rows.end(),
                      [](const QModelIndex &a, const QModelIndex &b) {
                        return a.row() > b.row();
                      });
            bool anyRemoved = false;
            for (const QModelIndex &idx : rows) {
              if (!idx.isValid())
                continue;
              // Skip substrate at row 0 (protected by model too)
              if (idx.row() == 0 && !idx.parent().isValid())
                continue;
              anyRemoved |= m->removeRow(idx.row(), idx.parent());
            }
            if (anyRemoved) {
              return true; // handled
            }
          }
        }
      }
    }
  }
  return QWidget::eventFilter(obj, event);
}
