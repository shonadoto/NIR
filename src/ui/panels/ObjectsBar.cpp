#include "ObjectsBar.h"

#include <QEvent>
#include <QGraphicsScene>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QModelIndex>
#include <QPushButton>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <algorithm>
#include <memory>

#include "model/MaterialModel.h"
#include "model/ObjectTreeModel.h"
#include "scene/ISceneObject.h"
#include "scene/items/CircleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/StickItem.h"
#include "ui/bindings/ShapeModelBinder.h"
#include "ui/editor/EditorArea.h"

namespace {

constexpr int kMinObjectsBarWidthPx = 220;
constexpr int kDefaultObjectsBarWidthPx = 280;

}  // namespace

ObjectsBar::ObjectsBar(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  setup_toolbar();
  layout->addWidget(toolbar_);

  tree_view_ = new QTreeView(this);
  layout->addWidget(tree_view_);
  tree_view_->setHeaderHidden(true);
  tree_view_->setEditTriggers(QAbstractItemView::EditKeyPressed |
                              QAbstractItemView::SelectedClicked |
                              QAbstractItemView::DoubleClicked);
  tree_view_->setExpandsOnDoubleClick(false);

  preferred_width_ = kDefaultObjectsBarWidthPx;
  last_visible_width_ = 0;
  setMinimumWidth(kMinObjectsBarWidthPx);
  setFixedWidth(preferred_width_);
}

void ObjectsBar::setup_toolbar() {
  toolbar_ = new QToolBar(this);
  toolbar_->setMovable(false);
  toolbar_->setFloatable(false);
  toolbar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  add_btn_ = new QPushButton("+", this);
  add_btn_->setToolTip("Add new item or material preset");
  add_btn_->setMaximumWidth(30);
  connect(add_btn_, &QPushButton::clicked, this,
          &ObjectsBar::add_item_or_preset);
  toolbar_->addWidget(add_btn_);

  remove_btn_ = new QPushButton("−", this);
  remove_btn_->setToolTip("Remove selected item or material preset");
  remove_btn_->setMaximumWidth(30);
  connect(remove_btn_, &QPushButton::clicked, this,
          &ObjectsBar::remove_selected_item);
  toolbar_->addWidget(remove_btn_);
}

void ObjectsBar::set_editor_area(EditorArea* editor_area) {
  editor_area_ = editor_area;
}

void ObjectsBar::set_shape_binder(ShapeModelBinder* binder) {
  shape_binder_ = binder;
}

void ObjectsBar::add_item_or_preset() {
  if (!tree_view_->model()) {
    return;
  }

  auto* model = qobject_cast<ObjectTreeModel*>(tree_view_->model());
  if (!model) {
    return;
  }

  if (auto* sel = tree_view_->selectionModel()) {
    QModelIndex current = sel->currentIndex();
    QModelIndex root;
    QModelIndex inclusionsIdx = model->index(0, 0, root);
    QModelIndex materialsIdx = model->index(1, 0, root);

    bool isInclusionsContext = false;
    bool isMaterialsContext = false;

    if (current.isValid()) {
      // Check if current selection is under Inclusions or Materials
      QModelIndex ancestor = current;
      while (ancestor.isValid() && ancestor.parent().isValid()) {
        ancestor = ancestor.parent();
      }
      if (ancestor == inclusionsIdx) {
        isInclusionsContext = true;
      } else if (ancestor == materialsIdx) {
        isMaterialsContext = true;
      } else if (ancestor == root || !ancestor.isValid()) {
        // If selected root or nothing, check which group node is selected
        if (current == inclusionsIdx) {
          isInclusionsContext = true;
        } else if (current == materialsIdx) {
          isMaterialsContext = true;
        } else {
          // Default to Inclusions if nothing clear
          isInclusionsContext = true;
        }
      }
    } else {
      // If nothing selected, default to Inclusions
      isInclusionsContext = true;
    }

    // If context is Inclusions, add a circle (default shape)
    if (isInclusionsContext) {
      if (!editor_area_) {
        return;
      }
      auto* scene = editor_area_->scene();
      if (!scene) {
        return;
      }
      const QPointF c = editor_area_->substrate_center();
      auto* circle = new CircleItem(40);
      scene->addItem(circle);
      circle->setPos(c);
      std::shared_ptr<ShapeModel> shapeModel;
      if (shape_binder_) {
        shapeModel = shape_binder_->bind_shape(circle);
      }
      // Select the new item
      if (shapeModel) {
        QModelIndex itemIdx = model->index_from_shape(shapeModel);
        if (itemIdx.isValid()) {
          tree_view_->selectionModel()->setCurrentIndex(
            itemIdx, QItemSelectionModel::ClearAndSelect);
          tree_view_->edit(itemIdx);  // Start editing name
        }
      }
    } else if (isMaterialsContext) {
      auto material = model->create_material(QStringLiteral("New Material"));
      tree_view_->expand(materialsIdx);
      QModelIndex materialIdx = model->index_from_material(material);
      if (materialIdx.isValid()) {
        tree_view_->selectionModel()->setCurrentIndex(
          materialIdx, QItemSelectionModel::ClearAndSelect);
        tree_view_->edit(materialIdx);
      }
    }
  }
}

void ObjectsBar::remove_selected_item() {
  if (!tree_view_->model()) {
    return;
  }

  auto* model = qobject_cast<ObjectTreeModel*>(tree_view_->model());
  if (!model) {
    return;
  }

  if (auto* sel = tree_view_->selectionModel()) {
    QModelIndex current = sel->currentIndex();
    if (!current.isValid()) {
      return;
    }

    // Check if it's a material preset
    auto material = model->material_from_index(current);
    if (material) {
      model->remove_material(material);
      return;
    }

    // Otherwise it's an inclusion item
    auto shape = model->shape_from_index(current);
    if (shape) {
      if (shape_binder_) {
        if (auto* item = shape_binder_->item_for(shape)) {
          shape_binder_->unbind_shape(dynamic_cast<ISceneObject*>(item));
        }
      }
      QModelIndex parent = current.parent();
      if (parent.isValid()) {
        model->removeRow(current.row(), parent);
      }
    }
  }
}

void ObjectsBar::update_button_states() {
  // Enable/disable buttons based on selection
  if (!tree_view_->model()) {
    add_btn_->setEnabled(false);
    remove_btn_->setEnabled(false);
    return;
  }

  add_btn_->setEnabled(true);

  if (auto* sel = tree_view_->selectionModel()) {
    QModelIndex current = sel->currentIndex();
    // Enable remove button only if a deletable item is selected
    remove_btn_->setEnabled(current.isValid() && current.parent().isValid());
  } else {
    remove_btn_->setEnabled(false);
  }
}

ObjectsBar::~ObjectsBar() = default;

void ObjectsBar::setPreferredWidth(int w) {
  preferred_width_ = w;
  setFixedWidth(preferred_width_);
}

void ObjectsBar::hideEvent(QHideEvent* event) {
  last_visible_width_ = width();
  QWidget::hideEvent(event);
}

void ObjectsBar::showEvent(QShowEvent* event) {
  const int target =
    last_visible_width_ > 0 ? last_visible_width_ : preferred_width_;
  setFixedWidth(target);
  QWidget::showEvent(event);
}

void ObjectsBar::set_model(QAbstractItemModel* model) {
  tree_view_->setModel(model);
  tree_view_->installEventFilter(this);
  // Expand root children by default
  if (model) {
    QModelIndex root;
    for (int i = 0; i < model->rowCount(root); ++i) {
      QModelIndex child = model->index(i, 0, root);
      if (child.isValid()) {
        tree_view_->expand(child);
      }
    }
    // Connect selection changes to update button states
    if (auto* sel = tree_view_->selectionModel()) {
      connect(sel, &QItemSelectionModel::selectionChanged, this,
              &ObjectsBar::update_button_states);
      connect(sel, &QItemSelectionModel::currentChanged, this,
              &ObjectsBar::update_button_states);
    }
  }
  update_button_states();
}

bool ObjectsBar::eventFilter(QObject* obj, QEvent* event) {
  if (obj == tree_view_) {
    if (event->type() == QEvent::KeyPress) {
      auto* ke = static_cast<QKeyEvent*>(event);
      if (ke->isAutoRepeat()) {
        return false;  // ignore auto-repeat to prevent deleting multiple items
                       // unintentionally
      }
      if (ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace) {
        if (auto* m = tree_view_->model()) {
          if (auto* sel = tree_view_->selectionModel()) {
            // Delete all selected rows (column 0) from bottom to top
            QList<QModelIndex> rows = sel->selectedRows(0);
            std::sort(rows.begin(), rows.end(),
                      [](const QModelIndex& a, const QModelIndex& b) {
                        return a.row() > b.row();
                      });
            bool anyRemoved = false;
            for (const QModelIndex& idx : rows) {
              if (!idx.isValid())
                continue;
              // Skip group nodes (Inclusions, Materials) - they cannot be
              // deleted
              if (!idx.parent().isValid())
                continue;
              anyRemoved |= m->removeRow(idx.row(), idx.parent());
            }
            if (anyRemoved) {
              return true;  // handled
            }
          }
        }
      }
    }
  }
  return QWidget::eventFilter(obj, event);
}
