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

  document_model_ = std::make_unique<DocumentModel>();
  shape_binder_ = std::make_unique<ShapeModelBinder>(*document_model_);

  createActivityObjectsBarAndEditor();
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
            if (tree_model_ == nullptr || current_selected_item_ == nullptr) {
              return;
            }
            if (shape_binder_ != nullptr && current_selected_item_ != nullptr) {
              auto model = shape_binder_->model_for(
                dynamic_cast<ISceneObject*>(current_selected_item_));
              if (model != nullptr) {
                // above
                const QModelIndex idx = tree_model_->index_from_shape(model);
                if (idx.isValid()) {
                  // namespace
                  tree_model_->setData(idx, new_name, Qt::EditRole);
                }
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
          &MainWindow::change_shape_type);
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
                if (item != nullptr && item == current_selected_item_) {
                  const QString name =
                    QString::fromStdString(shape_model->name());
                  properties_bar_->update_name(name);
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
                if (editor_area_ == nullptr) {
                  return;
                }
                auto shape_model = tree_model_->shape_from_index(current);
                if (shape_model != nullptr && shape_binder_ != nullptr) {
                  auto* item = shape_binder_->item_for(shape_model);
                  if (item != nullptr) {
                    auto* scene = editor_area_->scene();
                    if (scene == nullptr) {
                      return;
                    }
                    scene->clearSelection();
                    item->setSelected(true);
                    current_selected_item_ = item;
                    if (properties_bar_ != nullptr) {
                      if (auto* scene_obj = dynamic_cast<ISceneObject*>(item)) {
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
                  properties_bar_->set_selected_material(material.get());
                }
              });
      // Scene -> Tree selection
      if (auto* scene = editor_area_->scene()) {
        connect(
          scene, &QGraphicsScene::selectionChanged, this, [this, tree_view] {
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
            if (shape_binder_ != nullptr) {
              if (auto model = shape_binder_->model_for(
                    dynamic_cast<ISceneObject*>(items.first()))) {
                const QModelIndex idx = tree_model_->index_from_shape(model);
                if (idx.isValid()) {
                  tree_view->selectionModel()->setCurrentIndex(
                    idx, QItemSelectionModel::ClearAndSelect);
                }
              }
            }
            // Update properties bar
            if (properties_bar_ != nullptr) {
              if (auto* scene_obj =
                    dynamic_cast<ISceneObject*>(items.first())) {
                current_selected_item_ = items.first();
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
  // Clear all objects except substrate
  if (editor_area_ == nullptr) {
    return;
  }
  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  // Clear properties bar FIRST before deleting items
  if (properties_bar_ != nullptr) {
    properties_bar_->clear();
  }
  current_selected_item_ = nullptr;

  if (document_model_ != nullptr) {
    document_model_->clear_shapes();
    document_model_->clear_materials();
    auto substrate = std::make_shared<SubstrateModel>(
      Size2D{kDefaultSubstrateWidthPx, kDefaultSubstrateHeightPx},
      Color{kDefaultSubstrateColorR, kDefaultSubstrateColorG,
            kDefaultSubstrateColorB, kDefaultSubstrateColorA});
    substrate->set_name("Substrate");
    document_model_->set_substrate(substrate);
  }

  rebuild_scene_from_document();

  // Clear current file and selection
  current_file_path_.clear();
  current_selected_item_ = nullptr;

  // Show substrate in properties
  if (properties_bar_ != nullptr && editor_area_->substrate_item() != nullptr) {
    properties_bar_->set_selected_item(editor_area_->substrate_item(),
                                       "Substrate");
  }

  statusBar()->showMessage("New project created", kStatusBarMessageTimeoutMs);
}

void MainWindow::save_project() {
  if (current_file_path_.isEmpty()) {
    save_project_as();
    return;
  }

  sync_document_from_scene();
  if (ProjectSerializer::save_to_file(current_file_path_,
                                      document_model_.get())) {
    statusBar()->showMessage("Project saved successfully",
                             kStatusBarMessageTimeoutMs);
  } else {
    statusBar()->showMessage("Failed to save project",
                             kStatusBarMessageTimeoutMs);
  }
}

void MainWindow::save_project_as() {
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

  sync_document_from_scene();
  if (ProjectSerializer::save_to_file(filename, document_model_.get())) {
    current_file_path_ = filename;
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project saved successfully",
                             kStatusBarMessageTimeoutMs);
  } else {
    statusBar()->showMessage("Failed to save project",
                             kStatusBarMessageTimeoutMs);
  }
}

void MainWindow::open_project() {
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

  if (ProjectSerializer::load_from_file(filename, document_model_.get())) {
    rebuild_scene_from_document();
    current_file_path_ = filename;
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project loaded successfully",
                             kStatusBarMessageTimeoutMs);
  } else {
    statusBar()->showMessage("Failed to load project",
                             kStatusBarMessageTimeoutMs);
  }
}

void MainWindow::sync_document_from_scene() {
  if (document_model_ == nullptr || editor_area_ == nullptr) {
    return;
  }
  auto substrate = document_model_->substrate();
  if (!substrate) {
    substrate = std::make_shared<SubstrateModel>();
    document_model_->set_substrate(substrate);
  }
  if (auto* substrate_item = editor_area_->substrate_item()) {
    const QSizeF size = substrate_item->size();
    substrate->set_size(Size2D{size.width(), size.height()});
    substrate->set_color(to_model_color(substrate_item->fill_color()));
    substrate->set_name(substrate_item->name().toStdString());
  }
}

auto MainWindow::create_item_for_shape(const std::shared_ptr<ShapeModel>& shape)
  -> ISceneObject* {
  if (shape == nullptr) {
    return nullptr;
  }
  const Size2D size = shape->size();
  switch (shape->type()) {
    case ShapeModel::ShapeType::Rectangle: {
      auto* item = new RectangleItem(QRectF(0, 0, size.width, size.height));
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Ellipse: {
      auto* item = new EllipseItem(QRectF(0, 0, size.width, size.height));
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Circle: {
      const qreal radius = static_cast<qreal>(size.width) / 2.0;
      auto* item = new CircleItem(radius > 0 ? radius : kDefaultCircleRadius);
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Stick: {
      const qreal half_len = static_cast<qreal>(size.width) / 2.0;
      auto* item = new StickItem(QLineF(-half_len, 0.0, half_len, 0.0));
      QPen pen = item->pen();
      pen.setWidthF(ShapeConstants::kStickThickness);
      item->setPen(pen);
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
  }
  return nullptr;
}

void MainWindow::rebuild_scene_from_document() {
  if (document_model_ == nullptr || editor_area_ == nullptr ||
      shape_binder_ == nullptr) {
    return;
  }
  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  if (properties_bar_ != nullptr) {
    properties_bar_->clear();
  }
  current_selected_item_ = nullptr;

  shape_binder_->clear_bindings();

  QList<QGraphicsItem*> to_remove;
  for (QGraphicsItem* item : scene->items()) {
    if (dynamic_cast<SubstrateItem*>(item) == nullptr) {
      to_remove.append(item);
    }
  }
  for (QGraphicsItem* item : to_remove) {
    scene->removeItem(item);
    delete item;
  }

  if (auto substrate_model = document_model_->substrate()) {
    if (auto* substrate_item = editor_area_->substrate_item()) {
      substrate_item->set_size(
        QSizeF(substrate_model->size().width, substrate_model->size().height));
      substrate_item->set_fill_color(to_qcolor(substrate_model->color()));
      substrate_item->set_name(QString::fromStdString(substrate_model->name()));
    }
  }

  for (const auto& shape : document_model_->shapes()) {
    if (!shape) {
      continue;
    }
    if (auto* scene_obj = MainWindow::create_item_for_shape(shape)) {
      if (auto* graphics_item = dynamic_cast<QGraphicsItem*>(scene_obj)) {
        scene_obj->set_name(QString::fromStdString(shape->name()));
        graphics_item->setPos(shape->position().x, shape->position().y);
        graphics_item->setRotation(shape->rotation_deg());
        scene->addItem(graphics_item);
        shape_binder_->attach_shape(scene_obj, shape);
      } else {
        delete scene_obj;
      }
    }
  }

  tree_model_->set_substrate(editor_area_->substrate_item());
}

auto MainWindow::string_to_shape_type(const QString& type)
  -> ShapeModel::ShapeType {
  if (type == "circle") {
    return ShapeModel::ShapeType::Circle;
  }
  if (type == "ellipse") {
    return ShapeModel::ShapeType::Ellipse;
  }
  if (type == "stick") {
    return ShapeModel::ShapeType::Stick;
  }
  return ShapeModel::ShapeType::Rectangle;
}

auto MainWindow::convert_shape_size(ShapeModel::ShapeType from,
                                    ShapeModel::ShapeType target_type,
                                    const Size2D& size) -> Size2D {
  return ShapeSizeConverter::convert(from, target_type, size);
}

void MainWindow::change_shape_type(ISceneObject* old_item,
                                   const QString& new_type) {
  if (old_item == nullptr || editor_area_ == nullptr ||
      tree_model_ == nullptr) {
    return;
  }

  auto* old_graphics_item = dynamic_cast<QGraphicsItem*>(old_item);
  if (old_graphics_item == nullptr) {
    return;
  }

  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  std::shared_ptr<ShapeModel> shape_model;
  if (shape_binder_ != nullptr) {
    shape_model = shape_binder_->model_for(old_item);
  }
  if (shape_model == nullptr) {
    return;
  }

  const ShapeModel::ShapeType target_type =
    MainWindow::string_to_shape_type(new_type);
  if (shape_model->type() == target_type) {
    return;
  }

  // Get current properties before changing type
  // Calculate center of old item in scene coordinates using sceneBoundingRect
  // for accuracy
  const QPointF old_center = old_graphics_item->sceneBoundingRect().center();

  const qreal rotation = old_graphics_item->rotation();
  const QString item_name = old_item->name();
  const Size2D current_model_size = shape_model->size();
  const ShapeModel::ShapeType current_type = shape_model->type();

  // Convert size and ensure minimum
  Size2D new_size = MainWindow::convert_shape_size(current_type, target_type,
                                                   current_model_size);
  new_size = ShapeSizeConverter::ensure_minimum(new_size, target_type);

  // Update model
  shape_model->set_type(target_type);
  shape_model->set_size(new_size);

  // Replace the item in the scene, preserving center position
  replace_shape_item(old_item, shape_model, old_center, rotation, item_name);
}

void MainWindow::replace_shape_item(ISceneObject* old_item,
                                    const std::shared_ptr<ShapeModel>& model,
                                    const QPointF& center_position,
                                    qreal rotation, const QString& name) {
  if (old_item == nullptr || editor_area_ == nullptr || model == nullptr) {
    return;
  }

  auto* old_graphics_item = dynamic_cast<QGraphicsItem*>(old_item);
  if (old_graphics_item == nullptr) {
    return;
  }

  auto* scene = editor_area_->scene();
  if (scene == nullptr) {
    return;
  }

  // Create new item (not yet added to scene)
  std::unique_ptr<ISceneObject> new_item_ptr(
    MainWindow::create_item_for_shape(model));
  if (new_item_ptr == nullptr) {
    return;  // Failed to create item
  }

  auto* new_item = new_item_ptr.release();
  auto* new_graphics_item = dynamic_cast<QGraphicsItem*>(new_item);
  if (new_graphics_item == nullptr) {
    delete new_item;
    return;
  }

  // Calculate position for new item so its center matches the old center
  // We need to add item to scene temporarily to get accurate boundingRect
  // (some items may need scene context for proper boundingRect calculation)
  scene->addItem(new_graphics_item);
  const QRectF new_bounding_rect = new_graphics_item->boundingRect();
  scene->removeItem(new_graphics_item);

  // Validate boundingRect
  if (new_bounding_rect.isEmpty() || !new_bounding_rect.isValid()) {
    delete new_item;
    return;  // Invalid bounding rect
  }

  // Calculate position: center in scene = pos() + boundingRect().center()
  // So: pos() = center - boundingRect().center()
  const QPointF new_position = center_position - new_bounding_rect.center();

  // Unbind old item first, before removing it
  if (shape_binder_ != nullptr) {
    shape_binder_->unbind_shape(old_item);
  }

  // Remove old item from scene before adding new one
  scene->removeItem(old_graphics_item);
  delete old_graphics_item;

  // Set up new item with calculated position to preserve center
  new_item->set_name(name);
  new_graphics_item->setPos(new_position);
  new_graphics_item->setRotation(rotation);
  scene->addItem(new_graphics_item);

  // Update model position before binding to avoid apply_geometry overwriting it
  if (shape_binder_ != nullptr) {
    // Convert QPointF to model Point2D (using same conversion as
    // ShapeModelBinder)
    model->set_position(Point2D{new_position.x(), new_position.y()});
    model->set_rotation_deg(rotation);

    // Now bind - apply_geometry will use the correct position from model
    shape_binder_->attach_shape(new_item, model);
    // Force update to ensure correct rendering
    new_graphics_item->update();
  }

  // Update UI
  current_selected_item_ = new_graphics_item;
  if (properties_bar_ != nullptr) {
    properties_bar_->set_selected_item(new_item, name);
  }
}
