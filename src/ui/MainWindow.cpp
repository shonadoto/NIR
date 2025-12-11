#include "MainWindow.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLineF>
#include <QList>
#include <QMainWindow>
#include <QMenuBar>
#include <QModelIndex>
#include <QPen>
#include <QPointF>
#include <QSettings>
#include <QSize>
#include <QSizeF>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeView>
#include <QVector>
#include <Qt>
#include <memory>

#include "commands/CommandManager.h"
#include "model/DocumentModel.h"
#include "model/MaterialModel.h"
#include "model/ObjectTreeModel.h"
#include "model/ShapeModel.h"
#include "model/ShapeSizeConverter.h"
#include "model/SubstrateModel.h"
#include "model/core/ModelTypes.h"
#include "scene/ISceneObject.h"
#include "scene/items/CircleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/StickItem.h"
#include "serialization/ProjectSerializer.h"
#include "ui/bindings/ShapeModelBinder.h"
#include "ui/controller/DocumentController.h"
#include "ui/editor/EditorArea.h"
#include "ui/editor/SubstrateDialog.h"
#include "ui/editor/SubstrateItem.h"
#include "ui/panels/ObjectsBar.h"
#include "ui/panels/PropertiesBar.h"
#include "ui/sidebar/SideBarWidget.h"
#include "ui/utils/ColorUtils.h"

namespace {
constexpr int kDefaultObjectsBarWidthPx = 280;
constexpr int kDefaultWindowWidthPx = 1200;
constexpr int kDefaultWindowHeightPx = 800;
constexpr int kStatusBarMessageTimeoutMs = 3000;
constexpr double kDefaultSubstrateWidthPx = 1000.0;
constexpr double kDefaultSubstrateHeightPx = 1000.0;
constexpr int kDefaultSubstrateColorR = 240;
constexpr int kDefaultSubstrateColorG = 240;
constexpr int kDefaultSubstrateColorB = 240;
constexpr int kDefaultSubstrateColorA = 255;
constexpr double kDefaultCircleRadius = 50.0;
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle("NIR Material Editor");
  setWindowIcon(QIcon(":/icons/app.svg"));

  // Create document model and binder
  document_model_ = std::make_unique<DocumentModel>();
  shape_binder_ = std::make_unique<ShapeModelBinder>(*document_model_);

  // Create command manager
  command_manager_ = std::make_unique<CommandManager>(this);

  // Create controller
  document_controller_ = std::make_unique<DocumentController>(this);
  document_controller_->set_document_model(document_model_.get());
  document_controller_->set_shape_binder(shape_binder_.get());
  document_controller_->set_command_manager(command_manager_.get());

  createActivityObjectsBarAndEditor();

  // Set editor area in controller after it's created
  if (editor_area_ != nullptr) {
    document_controller_->set_editor_area(editor_area_);
  }

  createMenuBar();
  createActionsAndToolbar();

  resize(kDefaultWindowWidthPx, kDefaultWindowHeightPx);
}

MainWindow::~MainWindow() {
  if (tree_model_ != nullptr) {
    tree_model_->set_document(nullptr);
  }
  // Ensure properties bar clears before scene/items are destroyed
  // This must happen BEFORE editor_area_ is destroyed, as it contains the scene
  if (properties_bar_ != nullptr) {
    properties_bar_->clear();
    properties_bar_ = nullptr;
  }
  // Clear current selection reference
  current_selected_item_ = nullptr;
}

void MainWindow::createMenuBar() {
  auto* file_menu = menuBar()->addMenu("File");

  auto* new_action = new QAction("New", this);
  new_action->setShortcut(QKeySequence::New);
  connect(new_action, &QAction::triggered, this, &MainWindow::new_project);
  file_menu->addAction(new_action);

  file_menu->addSeparator();

  auto* save_action = new QAction("Save", this);
  save_action->setShortcut(QKeySequence::Save);
  connect(save_action, &QAction::triggered, this, &MainWindow::save_project);
  file_menu->addAction(save_action);

  auto* save_as_action = new QAction("Save As...", this);
  save_as_action->setShortcut(QKeySequence::SaveAs);
  connect(save_as_action, &QAction::triggered, this,
          &MainWindow::save_project_as);
  file_menu->addAction(save_as_action);

  auto* open_action = new QAction("Open...", this);
  open_action->setShortcut(QKeySequence::Open);
  connect(open_action, &QAction::triggered, this, &MainWindow::open_project);
  file_menu->addAction(open_action);

  file_menu->addSeparator();

  auto* quit_action = new QAction("Quit", this);
  quit_action->setShortcut(QKeySequence::Quit);
  connect(quit_action, &QAction::triggered, this, &QMainWindow::close);
  file_menu->addAction(quit_action);
}

void MainWindow::createActionsAndToolbar() {
  auto* toolbar = addToolBar("Tools");
  toolbar->setMovable(true);
  toolbar->setFloatable(false);
  // namespace
  toolbar->setContextMenuPolicy(Qt::PreventContextMenu);

  // Undo/Redo actions
  auto* undo_action = new QAction("Undo", this);
  undo_action->setShortcut(QKeySequence::Undo);
  undo_action->setEnabled(false);
  connect(undo_action, &QAction::triggered, this, [this] {
    if (command_manager_ != nullptr) {
      command_manager_->undo();
    }
  });
  connect(
    command_manager_.get(), &CommandManager::history_changed, this,
    [this, undo_action] {
      if (command_manager_ != nullptr) {
        undo_action->setEnabled(command_manager_->can_undo());
        const auto desc = command_manager_->undo_description();
        undo_action->setToolTip(
          desc.empty() ? "Undo" : ("Undo: " + QString::fromStdString(desc)));
      }
    });
  toolbar->addAction(undo_action);

  auto* redo_action = new QAction("Redo", this);
  redo_action->setShortcut(QKeySequence::Redo);
  redo_action->setEnabled(false);
  connect(redo_action, &QAction::triggered, this, [this] {
    if (command_manager_ != nullptr) {
      command_manager_->redo();
    }
  });
  connect(
    command_manager_.get(), &CommandManager::history_changed, this,
    [this, redo_action] {
      if (command_manager_ != nullptr) {
        redo_action->setEnabled(command_manager_->can_redo());
        const auto desc = command_manager_->redo_description();
        redo_action->setToolTip(
          desc.empty() ? "Redo" : ("Redo: " + QString::fromStdString(desc)));
      }
    });
  toolbar->addAction(redo_action);

  toolbar->addSeparator();

  auto* fit_action = new QAction("Fit to View", this);
  connect(fit_action, &QAction::triggered, this, [this] {
    if (editor_area_ != nullptr) {
      editor_area_->fit_to_substrate();
    }
  });
  toolbar->addAction(fit_action);

  auto* substrate_size_action = new QAction("Substrate Size...", this);
  connect(substrate_size_action, &QAction::triggered, this, [this] {
    if (editor_area_ == nullptr) {
      return;
    }
    const QSizeF cur = editor_area_->substrate_size();
    SubstrateDialog dlg(this, cur.width(), cur.height());
    if (dlg.exec() == QDialog::Accepted) {
      editor_area_->set_substrate_size(QSizeF(dlg.width_px(), dlg.height_px()));
    }
  });
  toolbar->addAction(substrate_size_action);

  // Shape creation is now handled in ObjectsBar via + button
}

void MainWindow::createActivityObjectsBarAndEditor() {
  // Qt uses parent-based ownership, not smart pointers
  // included above
  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(0);

  // Left: SideBarWidget with activity buttons and stack of bars
  side_bar_widget_ = new SideBarWidget(splitter);
  // Register default Objects bar
  auto* objects_bar = new ObjectsBar(side_bar_widget_);
  objects_bar->set_shape_binder(shape_binder_.get());
  objects_bar->set_command_manager(command_manager_.get());
  objects_bar->set_document_model(document_model_.get());
  side_bar_widget_->registerSidebar("objects", QIcon(":/icons/objects.svg"),
                                    objects_bar, kDefaultObjectsBarWidthPx);

  // Middle/Right: Editor area + Properties bar
  auto* right_splitter = new QSplitter(Qt::Horizontal, splitter);
  right_splitter->setChildrenCollapsible(false);
  right_splitter->setHandleWidth(1);

  editor_area_ = new EditorArea(right_splitter);
  properties_bar_ = new PropertiesBar(right_splitter);
  properties_bar_->set_shape_binder(shape_binder_.get());
  // Always visible, show substrate by default

  right_splitter->addWidget(editor_area_);
  right_splitter->addWidget(properties_bar_);
  right_splitter->setStretchFactor(0, 1);
  right_splitter->setStretchFactor(1, 0);

  // Object tree model (root + substrate)
  tree_model_ = new ObjectTreeModel(this);
  tree_model_->set_document(document_model_.get());
  tree_model_->set_substrate(editor_area_->substrate_item());

  // Connect ObjectsBar to EditorArea
  if (auto* objects_bar = side_bar_widget_->findChild<ObjectsBar*>()) {
    objects_bar->set_editor_area(editor_area_);
  }

  // Connect PropertiesBar to model
  properties_bar_->set_model(tree_model_);

  // Show substrate properties by default
  if (auto* substrate = editor_area_->substrate_item()) {
    properties_bar_->set_selected_item(substrate, "Substrate");
  }

  // Connect PropertiesBar name change to model update
  connect(properties_bar_, &PropertiesBar::name_changed, this,
          [this](const QString& new_name) {
            if (tree_model_ == nullptr || current_selected_item_ == nullptr ||
                shape_binder_ == nullptr) {
              return;
            }
            // Validate current_selected_item_ is still valid
            if (current_selected_item_->scene() == nullptr) {
              current_selected_item_ = nullptr;
              return;
            }
            auto* scene_obj =
              dynamic_cast<ISceneObject*>(current_selected_item_);
            if (scene_obj == nullptr) {
              return;
            }
            auto model = shape_binder_->model_for(scene_obj);
            if (model != nullptr) {
              const QModelIndex idx = tree_model_->index_from_shape(model);
              if (idx.isValid()) {
                tree_model_->setData(idx, new_name, Qt::EditRole);
              }
            }
          });

  // Connect PropertiesBar preset name change to model update
  connect(properties_bar_, &PropertiesBar::material_name_changed, this,
          [this](MaterialModel* material, const QString& new_name) {
            if (material == nullptr || tree_model_ == nullptr ||
                document_model_ == nullptr) {
              return;
            }
            std::shared_ptr<MaterialModel> shared;
            for (const auto& mat : document_model_->materials()) {
              if (mat.get() == material) {
                shared = mat;
                break;
              }
            }
            if (shared == nullptr) {
              return;
            }
            const QModelIndex idx = tree_model_->index_from_material(shared);
            if (idx.isValid()) {
              tree_model_->setData(idx, new_name, Qt::EditRole);
            }
          });

  // Connect PropertiesBar type change to replace object
  connect(properties_bar_, &PropertiesBar::type_changed, this,
          [this](ISceneObject* item, const QString& new_type) {
            if (document_controller_ == nullptr || item == nullptr) {
              return;
            }
            // Validate item is still valid
            if (auto* graphics_item = dynamic_cast<QGraphicsItem*>(item)) {
              if (graphics_item->scene() == nullptr) {
                return;
              }
            }
            document_controller_->change_shape_type(item, new_type);
          });
  // Bind model to the Objects bar (first sidebar entry)
  // We know SideBarWidget registered the "objects" page as index 0
  // so we can safely find the first page's widget and cast to ObjectsBar
  // Alternatively SideBarWidget could expose a getter; for now we search
  // children Connect model dataChanged to PropertiesBar update
  connect(tree_model_, &QAbstractItemModel::dataChanged, this,
          [this](const QModelIndex& top_left, const QModelIndex&,
                 const QVector<int>& roles) {
            if (roles.contains(Qt::DisplayRole) || roles.isEmpty()) {
              auto shape_model = tree_model_->shape_from_index(top_left);
              if (shape_model != nullptr && shape_binder_ != nullptr) {
                auto* item = shape_binder_->item_for(shape_model);
                // Validate item is still valid and matches current selection
                if (item != nullptr && item->scene() != nullptr &&
                    item == current_selected_item_) {
                  const QString name =
                    QString::fromStdString(shape_model->name());
                  if (properties_bar_ != nullptr) {
                    properties_bar_->update_name(name);
                  }
                }
              }
              // Check if it's a material node
              auto material = tree_model_->material_from_index(top_left);
              if (material != nullptr && properties_bar_ != nullptr) {
                // Could refresh material view if needed
              }
            }
          });

  if (auto* objects_bar = side_bar_widget_->findChild<ObjectsBar*>()) {
    objects_bar->set_model(tree_model_);
    auto* tree_view = objects_bar->treeView();
    if (tree_view != nullptr) {
      // Tree -> Scene selection
      // above
      connect(tree_view->selectionModel(), &QItemSelectionModel::currentChanged,
              this, [this](const QModelIndex& current, const QModelIndex&) {
                if (editor_area_ == nullptr || tree_model_ == nullptr) {
                  return;
                }
                auto shape_model = tree_model_->shape_from_index(current);
                if (shape_model != nullptr && shape_binder_ != nullptr) {
                  auto* item = shape_binder_->item_for(shape_model);
                  if (item != nullptr) {
                    // Validate item is still in scene
                    if (item->scene() == nullptr) {
                      return;
                    }
                    auto* scene = editor_area_->scene();
                    if (scene == nullptr) {
                      return;
                    }
                    scene->clearSelection();
                    item->setSelected(true);
                    current_selected_item_ = item;
                    if (properties_bar_ != nullptr) {
                      auto* scene_obj = dynamic_cast<ISceneObject*>(item);
                      if (scene_obj != nullptr) {
                        const QString name =
                          QString::fromStdString(shape_model->name());
                        properties_bar_->set_selected_item(scene_obj, name);
                      }
                    }
                    return;
                  }
                }
                // Check if it's a material
                auto material = tree_model_->material_from_index(current);
                if (material != nullptr && properties_bar_ != nullptr) {
                  current_selected_item_ = nullptr;
                  // Clear scene selection when selecting material
                  if (editor_area_ != nullptr) {
                    auto* scene = editor_area_->scene();
                    if (scene != nullptr) {
                      scene->clearSelection();
                    }
                  }
                  properties_bar_->set_selected_material(material.get());
                }
              });
      // Scene -> Tree selection
      if (auto* scene = editor_area_->scene()) {
        connect(
          scene, &QGraphicsScene::selectionChanged, this, [this, tree_view] {
            if (editor_area_ == nullptr || tree_model_ == nullptr) {
              return;
            }
            auto items = editor_area_->scene()->selectedItems();
            if (items.isEmpty()) {
              // Show substrate when nothing selected
              if (properties_bar_ != nullptr &&
                  editor_area_->substrate_item() != nullptr) {
                properties_bar_->set_selected_item(
                  editor_area_->substrate_item(), "Substrate");
              }
              return;
            }
            // Validate first item is still valid
            auto* first_item = items.first();
            if (first_item == nullptr || first_item->scene() == nullptr) {
              return;
            }
            if (shape_binder_ != nullptr) {
              auto* scene_obj = dynamic_cast<ISceneObject*>(first_item);
              if (scene_obj != nullptr) {
                if (auto model = shape_binder_->model_for(scene_obj)) {
                  const QModelIndex idx = tree_model_->index_from_shape(model);
                  if (idx.isValid() && tree_view->selectionModel() != nullptr) {
                    tree_view->selectionModel()->setCurrentIndex(
                      idx, QItemSelectionModel::ClearAndSelect);
                  }
                }
              }
            }
            // Update properties bar
            if (properties_bar_ != nullptr) {
              auto* scene_obj = dynamic_cast<ISceneObject*>(first_item);
              if (scene_obj != nullptr) {
                current_selected_item_ = first_item;
                const QString name = scene_obj->name();
                properties_bar_->set_selected_item(scene_obj, name);
              }
            }
          });
      }
    }
  }

  // Put into main splitter
  splitter->addWidget(side_bar_widget_);
  splitter->addWidget(right_splitter);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  QList<int> sizes;  // NOLINT(misc-include-cleaner) QList is included above
  sizes << side_bar_widget_->width() << kDefaultWindowWidthPx;
  splitter->setSizes(sizes);

  setCentralWidget(splitter);
}

void MainWindow::new_project() {
  // Clear properties bar FIRST before deleting items
  if (properties_bar_ != nullptr) {
    properties_bar_->clear();
  }
  current_selected_item_ = nullptr;

  if (document_controller_ != nullptr) {
    document_controller_->new_document();
  }

  // Show substrate in properties
  if (properties_bar_ != nullptr && editor_area_ != nullptr &&
      editor_area_->substrate_item() != nullptr) {
    properties_bar_->set_selected_item(editor_area_->substrate_item(),
                                       "Substrate");
  }

  statusBar()->showMessage("New project created", kStatusBarMessageTimeoutMs);
}

void MainWindow::save_project() {
  if (document_controller_ == nullptr) {
    return;
  }

  const QString file_path = document_controller_->current_file_path();
  if (file_path.isEmpty()) {
    save_project_as();
    return;
  }

  if (document_controller_->save_document(file_path)) {
    statusBar()->showMessage("Project saved successfully",
                             kStatusBarMessageTimeoutMs);
  } else {
    statusBar()->showMessage("Failed to save project",
                             kStatusBarMessageTimeoutMs);
  }
}

void MainWindow::save_project_as() {
  if (document_controller_ == nullptr) {
    return;
  }

  QSettings settings("NIR", "MaterialEditor");
  const QString last_dir =
    settings.value("lastDirectory", QDir::homePath()).toString();
  const QString default_path = last_dir + "/untitled.json";

  const QString filename = QFileDialog::getSaveFileName(
    this, "Save Project As", default_path, "JSON Files (*.json)", nullptr,
    QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty()) {
    return;
  }

  if (document_controller_->save_document(filename)) {
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project saved successfully",
                             kStatusBarMessageTimeoutMs);
  } else {
    statusBar()->showMessage("Failed to save project",
                             kStatusBarMessageTimeoutMs);
  }
}

void MainWindow::open_project() {
  if (document_controller_ == nullptr) {
    return;
  }

  QSettings settings("NIR", "MaterialEditor");
  const QString last_dir =
    settings.value("lastDirectory", QDir::homePath()).toString();

  const QString filename = QFileDialog::getOpenFileName(
    this, "Open Project", last_dir, "JSON Files (*.json)", nullptr,
    QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty()) {
    return;
  }

  // Clear properties bar before loading to avoid accessing deleted items
  if (properties_bar_ != nullptr) {
    properties_bar_->clear();
  }
  current_selected_item_ = nullptr;

  if (document_controller_->load_document(filename)) {
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project loaded successfully",
                             kStatusBarMessageTimeoutMs);
    // Show substrate in properties by default after loading
    if (properties_bar_ != nullptr && editor_area_ != nullptr &&
        editor_area_->substrate_item() != nullptr) {
      properties_bar_->set_selected_item(editor_area_->substrate_item(),
                                         "Substrate");
    }
  } else {
    statusBar()->showMessage("Failed to load project",
                             kStatusBarMessageTimeoutMs);
  }
}
