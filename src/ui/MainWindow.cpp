#include "MainWindow.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLineF>
#include <QMenuBar>
#include <QPen>
#include <QSettings>
#include <QSize>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeView>

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
  auto* fileMenu = menuBar()->addMenu("File");

  auto* newAction = new QAction("New", this);
  newAction->setShortcut(QKeySequence::New);
  connect(newAction, &QAction::triggered, this, &MainWindow::new_project);
  fileMenu->addAction(newAction);

  fileMenu->addSeparator();

  auto* saveAction = new QAction("Save", this);
  saveAction->setShortcut(QKeySequence::Save);
  connect(saveAction, &QAction::triggered, this, &MainWindow::save_project);
  fileMenu->addAction(saveAction);

  auto* saveAsAction = new QAction("Save As...", this);
  saveAsAction->setShortcut(QKeySequence::SaveAs);
  connect(saveAsAction, &QAction::triggered, this,
          &MainWindow::save_project_as);
  fileMenu->addAction(saveAsAction);

  auto* openAction = new QAction("Open...", this);
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::open_project);
  fileMenu->addAction(openAction);

  fileMenu->addSeparator();

  auto* quitAction = new QAction("Quit", this);
  quitAction->setShortcut(QKeySequence::Quit);
  connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
  fileMenu->addAction(quitAction);
}

void MainWindow::createActionsAndToolbar() {
  auto* toolbar = addToolBar("Tools");
  toolbar->setMovable(true);
  toolbar->setFloatable(false);
  toolbar->setContextMenuPolicy(Qt::PreventContextMenu);

  auto* fitAction = new QAction("Fit to View", this);
  connect(fitAction, &QAction::triggered, this, [this] {
    if (editor_area_ != nullptr) {
      editor_area_->fit_to_substrate();
    }
  });
  toolbar->addAction(fitAction);

  auto* substrateSizeAction = new QAction("Substrate Size...", this);
  connect(substrateSizeAction, &QAction::triggered, this, [this] {
    if (editor_area_ == nullptr) {
      return;
    }
    const QSizeF cur = editor_area_->substrate_size();
    SubstrateDialog dlg(this, cur.width(), cur.height());
    if (dlg.exec() == QDialog::Accepted) {
      editor_area_->set_substrate_size(QSizeF(dlg.width_px(), dlg.height_px()));
    }
  });
  toolbar->addAction(substrateSizeAction);

  // Shape creation is now handled in ObjectsBar via + button
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void MainWindow::createActivityObjectsBarAndEditor() {
  // Qt uses parent-based ownership, not smart pointers
  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(0);

  // Left: SideBarWidget with activity buttons and stack of bars
  side_bar_widget_ = new SideBarWidget(splitter);
  // Register default Objects bar
  auto* objectsBar = new ObjectsBar(side_bar_widget_);
  objectsBar->set_shape_binder(shape_binder_.get());
  side_bar_widget_->registerSidebar("objects", QIcon(":/icons/objects.svg"),
                                    objectsBar, kDefaultObjectsBarWidthPx);

  // Middle/Right: Editor area + Properties bar
  auto* rightSplitter = new QSplitter(Qt::Horizontal, splitter);
  rightSplitter->setChildrenCollapsible(false);
  rightSplitter->setHandleWidth(1);

  editor_area_ = new EditorArea(rightSplitter);
  properties_bar_ = new PropertiesBar(rightSplitter);
  properties_bar_->set_shape_binder(shape_binder_.get());
  // Always visible, show substrate by default

  rightSplitter->addWidget(editor_area_);
  rightSplitter->addWidget(properties_bar_);
  rightSplitter->setStretchFactor(0, 1);
  rightSplitter->setStretchFactor(1, 0);

  // Object tree model (root + substrate)
  tree_model_ = new ObjectTreeModel(this);
  tree_model_->set_document(document_model_.get());
  tree_model_->set_substrate(editor_area_->substrate_item());

  // Connect ObjectsBar to EditorArea
  if (auto* objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
    objectsBar->set_editor_area(editor_area_);
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
                const QModelIndex idx = tree_model_->index_from_shape(model);
                if (idx.isValid()) {
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
          [this](const QModelIndex& topLeft, const QModelIndex&,
                 const QVector<int>& roles) {
            if (roles.contains(Qt::DisplayRole) || roles.isEmpty()) {
              auto shapeModel = tree_model_->shape_from_index(topLeft);
              if (shapeModel != nullptr && shape_binder_ != nullptr) {
                auto* item = shape_binder_->item_for(shapeModel);
                if (item != nullptr && item == current_selected_item_) {
                  const QString name = QString::fromStdString(shapeModel->name());
                  properties_bar_->update_name(name);
                }
              }
              // Check if it's a material node
              auto material = tree_model_->material_from_index(topLeft);
              if (material != nullptr && properties_bar_ != nullptr) {
                // Could refresh material view if needed
              }
            }
          });

  if (auto* objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
    objectsBar->set_model(tree_model_);
    auto* treeView = objectsBar->treeView();
    if (treeView != nullptr) {
      // Tree -> Scene selection
      connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged,
              this, [this](const QModelIndex& current, const QModelIndex&) {
                if (editor_area_ == nullptr) {
                  return;
                }
                auto shapeModel = tree_model_->shape_from_index(current);
                if (shapeModel != nullptr && shape_binder_ != nullptr) {
                  auto* item = shape_binder_->item_for(shapeModel);
                  if (item != nullptr) {
                    auto* scene = editor_area_->scene();
                    if (scene == nullptr) {
                      return;
                    }
                    scene->clearSelection();
                    item->setSelected(true);
                    current_selected_item_ = item;
                    if (properties_bar_ != nullptr) {
                      if (auto* sceneObj = dynamic_cast<ISceneObject*>(item)) {
                        const QString name =
                          QString::fromStdString(shapeModel->name());
                        properties_bar_->set_selected_item(sceneObj, name);
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
          scene, &QGraphicsScene::selectionChanged, this, [this, treeView] {
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
                  treeView->selectionModel()->setCurrentIndex(
                    idx, QItemSelectionModel::ClearAndSelect);
                }
              }
            }
            // Update properties bar
            if (properties_bar_ != nullptr) {
              if (auto* sceneObj = dynamic_cast<ISceneObject*>(items.first())) {
                current_selected_item_ = items.first();
                const QString name = sceneObj->name();
                properties_bar_->set_selected_item(sceneObj, name);
              }
            }
          });
      }
    }
  }

  // Put into main splitter
  splitter->addWidget(side_bar_widget_);
  splitter->addWidget(rightSplitter);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  QList<int> sizes;
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
  const QString lastDir =
    settings.value("lastDirectory", QDir::homePath()).toString();
  const QString defaultPath = lastDir + "/untitled.json";

  const QString filename = QFileDialog::getSaveFileName(
    this, "Save Project As", defaultPath, "JSON Files (*.json)", nullptr,
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
  const QString lastDir =
    settings.value("lastDirectory", QDir::homePath()).toString();

  const QString filename = QFileDialog::getOpenFileName(
    this, "Open Project", lastDir, "JSON Files (*.json)", nullptr,
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

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
ISceneObject* MainWindow::create_item_for_shape(
  const std::shared_ptr<ShapeModel>& shape) {
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
      const qreal halfLen = static_cast<qreal>(size.width) / 2.0;
      auto* item = new StickItem(QLineF(-halfLen, 0.0, halfLen, 0.0));
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

  QList<QGraphicsItem*> toRemove;
  for (QGraphicsItem* item : scene->items()) {
    if (dynamic_cast<SubstrateItem*>(item) == nullptr) {
      toRemove.append(item);
    }
  }
  for (QGraphicsItem* item : toRemove) {
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
    if (auto* sceneObj = create_item_for_shape(shape)) {
      if (auto* graphicsItem = dynamic_cast<QGraphicsItem*>(sceneObj)) {
        sceneObj->set_name(QString::fromStdString(shape->name()));
        graphicsItem->setPos(shape->position().x, shape->position().y);
        graphicsItem->setRotation(shape->rotation_deg());
        scene->addItem(graphicsItem);
        shape_binder_->attach_shape(sceneObj, shape);
      } else {
        delete sceneObj;
      }
    }
  }

  tree_model_->set_substrate(editor_area_->substrate_item());
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
ShapeModel::ShapeType MainWindow::string_to_shape_type(
  const QString& type) const {
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

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto MainWindow::convert_shape_size(ShapeModel::ShapeType from,
                                    ShapeModel::ShapeType target_type,
                                    const Size2D& size) const -> Size2D {
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

  std::shared_ptr<ShapeModel> shapeModel;
  if (shape_binder_ != nullptr) {
    shapeModel = shape_binder_->model_for(old_item);
  }
  if (shapeModel == nullptr) {
    return;
  }

  const ShapeModel::ShapeType targetType = string_to_shape_type(new_type);
  if (shapeModel->type() == targetType) {
    return;
  }

  // Get current properties before changing type
  // Calculate center of old item in scene coordinates using sceneBoundingRect
  // for accuracy
  const QPointF oldCenter = old_graphics_item->sceneBoundingRect().center();

  const qreal rotation = old_graphics_item->rotation();
  const QString item_name = old_item->name();
  const Size2D currentModelSize = shapeModel->size();
  const ShapeModel::ShapeType currentType = shapeModel->type();

  // Convert size and ensure minimum
  Size2D newSize =
    convert_shape_size(currentType, targetType, currentModelSize);
  newSize = ShapeSizeConverter::ensure_minimum(newSize, targetType);

  // Update model
  shapeModel->set_type(targetType);
  shapeModel->set_size(newSize);

  // Replace the item in the scene, preserving center position
  replace_shape_item(old_item, shapeModel, oldCenter, rotation, item_name);
}

void MainWindow::replace_shape_item(ISceneObject* old_item,
                                    const std::shared_ptr<ShapeModel>& model,
                                    const QPointF& centerPosition,
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
  std::unique_ptr<ISceneObject> new_item_ptr(create_item_for_shape(model));
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
  const QRectF newBoundingRect = new_graphics_item->boundingRect();
  scene->removeItem(new_graphics_item);

  // Validate boundingRect
  if (newBoundingRect.isEmpty() || !newBoundingRect.isValid()) {
    delete new_item;
    return;  // Invalid bounding rect
  }

  // Calculate position: center in scene = pos() + boundingRect().center()
  // So: pos() = center - boundingRect().center()
  const QPointF newPosition = centerPosition - newBoundingRect.center();

  // Unbind old item first, before removing it
  if (shape_binder_ != nullptr) {
    shape_binder_->unbind_shape(old_item);
  }

  // Remove old item from scene before adding new one
  scene->removeItem(old_graphics_item);
  delete old_graphics_item;

  // Set up new item with calculated position to preserve center
  new_item->set_name(name);
  new_graphics_item->setPos(newPosition);
  new_graphics_item->setRotation(rotation);
  scene->addItem(new_graphics_item);

  // Update model position before binding to avoid apply_geometry overwriting it
  if (shape_binder_ != nullptr) {
    // Convert QPointF to model Point2D (using same conversion as
    // ShapeModelBinder)
    model->set_position(Point2D{newPosition.x(), newPosition.y()});
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
