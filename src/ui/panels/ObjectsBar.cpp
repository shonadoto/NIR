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
constexpr int kToolbarButtonWidthPx = 30;

}  // namespace

ObjectsBar::ObjectsBar(QWidget* parent)
    : QWidget(parent),
      // Qt uses parent-based ownership, not smart pointers
      tree_view_(new QTreeView(this)),
      preferred_width_(kDefaultObjectsBarWidthPx),
      last_visible_width_(0) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  setup_toolbar();
  layout->addWidget(toolbar_);

  layout->addWidget(tree_view_);
  tree_view_->setHeaderHidden(true);
  tree_view_->setEditTriggers(QAbstractItemView::EditKeyPressed |
                              QAbstractItemView::SelectedClicked |
                              QAbstractItemView::DoubleClicked);
  tree_view_->setExpandsOnDoubleClick(false);

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
  add_btn_->setMaximumWidth(kToolbarButtonWidthPx);
  connect(add_btn_, &QPushButton::clicked, this,
          &ObjectsBar::add_item_or_preset);
  toolbar_->addWidget(add_btn_);

  remove_btn_ = new QPushButton("−", this);
  remove_btn_->setToolTip("Remove selected item or material preset");
  remove_btn_->setMaximumWidth(kToolbarButtonWidthPx);
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void ObjectsBar::add_item_or_preset() {
  if (tree_view_->model() == nullptr) {
    return;
  }

  auto* model = qobject_cast<ObjectTreeModel*>(tree_view_->model());
  if (model == nullptr) {
    return;
  }

  if (auto* sel = tree_view_->selectionModel()) {
    const QModelIndex current = sel->currentIndex();
    const QModelIndex root;
    const QModelIndex inclusionsIdx = model->index(0, 0, root);
    const QModelIndex materialsIdx = model->index(1, 0, root);

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
        }
        // Default to Inclusions if nothing clear (handled below)
      }
    } else {
      // If nothing selected, default to Inclusions
      isInclusionsContext = true;
    }

    // If context is Inclusions, add a circle (default shape)
    if (isInclusionsContext) {
      if (editor_area_ == nullptr) {
        return;
      }
      auto* scene = editor_area_->scene();
      if (scene == nullptr) {
        return;
      }
      constexpr double kDefaultNewCircleRadius = 40.0;
      const QPointF center = editor_area_->substrate_center();
      auto* circle = new CircleItem(kDefaultNewCircleRadius);
      scene->addItem(circle);
      circle->setPos(center);
      std::shared_ptr<ShapeModel> shapeModel;
      if (shape_binder_ != nullptr) {
        shapeModel = shape_binder_->bind_shape(circle);
      }
      // Select the new item
      if (shapeModel) {
        const QModelIndex itemIdx = model->index_from_shape(shapeModel);
        if (itemIdx.isValid()) {
          tree_view_->selectionModel()->setCurrentIndex(
            itemIdx, QItemSelectionModel::ClearAndSelect);
          tree_view_->edit(itemIdx);  // Start editing name
        }
      }
    } else if (isMaterialsContext) {
      auto material = model->create_material(QStringLiteral("New Material"));
      tree_view_->expand(materialsIdx);
      const QModelIndex materialIdx = model->index_from_material(material);
      if (materialIdx.isValid()) {
        tree_view_->selectionModel()->setCurrentIndex(
          materialIdx, QItemSelectionModel::ClearAndSelect);
        tree_view_->edit(materialIdx);
      }
    }
  }
}

void ObjectsBar::remove_selected_item() {
  if (tree_view_->model() == nullptr) {
    return;
  }

  auto* model = qobject_cast<ObjectTreeModel*>(tree_view_->model());
  if (model == nullptr) {
    return;
  }

  if (auto* sel = tree_view_->selectionModel()) {
    const QModelIndex current = sel->currentIndex();
    if (!current.isValid()) {
      return;
    }

    // Check if it's a material preset
    auto material = model->material_from_index(current);
    if (material != nullptr) {
      model->remove_material(material);
      return;
    }

    // Otherwise it's an inclusion item
    auto shape = model->shape_from_index(current);
    if (shape != nullptr) {
      if (shape_binder_ != nullptr) {
        if (auto* item = shape_binder_->item_for(shape)) {
          shape_binder_->unbind_shape(dynamic_cast<ISceneObject*>(item));
        }
      }
      const QModelIndex parent = current.parent();
      if (parent.isValid()) {
        model->removeRow(current.row(), parent);
      }
    }
  }
}

void ObjectsBar::update_button_states() {
  // Enable/disable buttons based on selection
  if (tree_view_->model() == nullptr) {
    add_btn_->setEnabled(false);
    remove_btn_->setEnabled(false);
    return;
  }

  add_btn_->setEnabled(true);

  if (auto* sel = tree_view_->selectionModel()) {
    const QModelIndex current = sel->currentIndex();
    // Enable remove button only if a deletable item is selected
    remove_btn_->setEnabled(current.isValid() && current.parent().isValid());
  } else {
    remove_btn_->setEnabled(false);
  }
}

ObjectsBar::~ObjectsBar() = default;

void ObjectsBar::setPreferredWidth(int width) {
  preferred_width_ = width;
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
  if (model != nullptr) {
    const QModelIndex root;
    for (int i = 0; i < model->rowCount(root); ++i) {
      const QModelIndex child = model->index(i, 0, root);
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool ObjectsBar::eventFilter(QObject* obj, QEvent* event) {
  if (obj == tree_view_) {
    if (event->type() == QEvent::KeyPress) {
      // QKeyEvent is not polymorphic, static_cast is safe here
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast,readability-identifier-length)
      auto* key_event = static_cast<QKeyEvent*>(event);
      if (key_event->isAutoRepeat()) {
        return false;  // ignore auto-repeat to prevent deleting multiple items
                       // unintentionally
      }
      if (key_event->key() == Qt::Key_Delete ||
          key_event->key() == Qt::Key_Backspace) {
        if (auto* model = tree_view_->model()) {
          if (auto* sel = tree_view_->selectionModel()) {
            // Delete all selected rows (column 0) from bottom to top
            QList<QModelIndex> rows = sel->selectedRows(0);
            std::sort(
              rows.begin(), rows.end(),
              [](const QModelIndex& index_a, const QModelIndex& index_b) {
                return index_a.row() > index_b.row();
              });
            bool anyRemoved = false;
            for (const QModelIndex& idx : rows) {
              if (!idx.isValid()) {
                continue;
              }
              // Skip group nodes (Inclusions, Materials) - they cannot be
              // deleted
              if (!idx.parent().isValid()) {
                continue;
              }
              anyRemoved |= model->removeRow(idx.row(), idx.parent());
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
