#include "MainWindow.h"

#include <QAction>
#include <QIcon>
#include <QSize>
#include <QSplitter>
#include <QToolBar>
#include <QTreeView>
#include <QGraphicsScene>
#include <QLineF>
#include <QPen>
#include <algorithm>
#include <QItemSelectionModel>
#include "ui/editor/EditorArea.h"
#include "ui/panels/ObjectsBar.h"
#include "ui/sidebar/SideBarWidget.h"
#include "ui/editor/SubstrateDialog.h"
#include "ui/editor/SubstrateItem.h"
#include "ui/panels/PropertiesBar.h"
#include "model/ObjectTreeModel.h"
#include "model/MaterialModel.h"
#include "model/ShapeModel.h"
#include "model/DocumentModel.h"
#include "model/SubstrateModel.h"
#include "model/core/ModelTypes.h"
#include "scene/ISceneObject.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/CircleItem.h"
#include "scene/items/StickItem.h"
#include "serialization/ProjectSerializer.h"
#include "ui/bindings/ShapeModelBinder.h"
#include "ui/utils/ColorUtils.h"
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QStatusBar>
#include <QDir>
#include <QSettings>
#include <QFileInfo>

namespace {
constexpr int kDefaultObjectsBarWidthPx = 280;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("NIR Material Editor");
  setWindowIcon(QIcon(":/icons/app.svg"));

  document_model_ = std::make_unique<DocumentModel>();
  shape_binder_ = std::make_unique<ShapeModelBinder>(*document_model_);

  createActivityObjectsBarAndEditor();
  createMenuBar();
  createActionsAndToolbar();

  resize(1200, 800);
}

MainWindow::~MainWindow() {
    if (tree_model_) {
        tree_model_->set_document(nullptr);
    }
    // Ensure properties bar clears before scene/items are destroyed
    // This must happen BEFORE editor_area_ is destroyed, as it contains the scene
    if (properties_bar_) {
        properties_bar_->clear();
        properties_bar_ = nullptr;
    }
    // Clear current selection reference
    current_selected_item_ = nullptr;
}

void MainWindow::createMenuBar() {
  auto *fileMenu = menuBar()->addMenu("File");

  auto *newAction = new QAction("New", this);
  newAction->setShortcut(QKeySequence::New);
  connect(newAction, &QAction::triggered, this, &MainWindow::new_project);
  fileMenu->addAction(newAction);

  fileMenu->addSeparator();

  auto *saveAction = new QAction("Save", this);
  saveAction->setShortcut(QKeySequence::Save);
  connect(saveAction, &QAction::triggered, this, &MainWindow::save_project);
  fileMenu->addAction(saveAction);

  auto *saveAsAction = new QAction("Save As...", this);
  saveAsAction->setShortcut(QKeySequence::SaveAs);
  connect(saveAsAction, &QAction::triggered, this, &MainWindow::save_project_as);
  fileMenu->addAction(saveAsAction);

  auto *openAction = new QAction("Open...", this);
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::open_project);
  fileMenu->addAction(openAction);

  fileMenu->addSeparator();

  auto *quitAction = new QAction("Quit", this);
  quitAction->setShortcut(QKeySequence::Quit);
  connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
  fileMenu->addAction(quitAction);
}

void MainWindow::createActionsAndToolbar() {
  auto *toolbar = addToolBar("Tools");
  toolbar->setMovable(true);
  toolbar->setFloatable(false);
  toolbar->setContextMenuPolicy(Qt::PreventContextMenu);

  auto *fitAction = new QAction("Fit to View", this);
  connect(fitAction, &QAction::triggered, this, [this]{
    if (editor_area_) {
      editor_area_->fit_to_substrate();
    }
  });
  toolbar->addAction(fitAction);

  auto *substrateSizeAction = new QAction("Substrate Size...", this);
  connect(substrateSizeAction, &QAction::triggered, this, [this]{
    if (!editor_area_) { return; }
    const QSizeF cur = editor_area_->substrate_size();
    SubstrateDialog dlg(this, cur.width(), cur.height());
    if (dlg.exec() == QDialog::Accepted) {
      editor_area_->set_substrate_size(QSizeF(dlg.width_px(), dlg.height_px()));
    }
  });
  toolbar->addAction(substrateSizeAction);

  // Shape creation is now handled in ObjectsBar via + button
}

void MainWindow::createActivityObjectsBarAndEditor() {
  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(0);

  // Left: SideBarWidget with activity buttons and stack of bars
  side_bar_widget_ = new SideBarWidget(splitter);
  // Register default Objects bar
  auto *objectsBar = new ObjectsBar(side_bar_widget_);
  objectsBar->set_shape_binder(shape_binder_.get());
  side_bar_widget_->registerSidebar(
      "objects", QIcon(":/icons/objects.svg"),
      objectsBar, kDefaultObjectsBarWidthPx);

  // Middle/Right: Editor area + Properties bar
  auto *rightSplitter = new QSplitter(Qt::Horizontal, splitter);
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
  if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
    objectsBar->set_editor_area(editor_area_);
  }

  // Connect PropertiesBar to model
  properties_bar_->set_model(tree_model_);

  // Show substrate properties by default
  if (auto *substrate = editor_area_->substrate_item()) {
    properties_bar_->set_selected_item(substrate, "Substrate");
  }

  // Connect PropertiesBar name change to model update
  connect(properties_bar_, &PropertiesBar::name_changed, this, [this](const QString &new_name){
    if (!tree_model_ || !current_selected_item_) return;
    if (shape_binder_ && current_selected_item_) {
      auto model = shape_binder_->model_for(dynamic_cast<ISceneObject*>(current_selected_item_));
      if (model) {
        QModelIndex idx = tree_model_->index_from_shape(model);
        if (idx.isValid()) {
          tree_model_->setData(idx, new_name, Qt::EditRole);
        }
      }
    }
  });

  // Connect PropertiesBar preset name change to model update
  connect(properties_bar_, &PropertiesBar::material_name_changed, this, [this](MaterialModel *material, const QString &new_name){
    if (!material || !tree_model_ || !document_model_) return;
    std::shared_ptr<MaterialModel> shared;
    for (const auto &mat : document_model_->materials()) {
      if (mat.get() == material) {
        shared = mat;
        break;
      }
    }
    if (!shared) return;
    QModelIndex idx = tree_model_->index_from_material(shared);
    if (idx.isValid()) {
      tree_model_->setData(idx, new_name, Qt::EditRole);
    }
  });

  // Connect PropertiesBar type change to replace object
  connect(properties_bar_, &PropertiesBar::type_changed, this, [this](ISceneObject *old_item, const QString &new_type){
    if (!old_item || !editor_area_ || !tree_model_) return;
    auto *old_graphics_item = dynamic_cast<QGraphicsItem*>(old_item);
    if (!old_graphics_item) return;

    auto *scene = editor_area_->scene();
    if (!scene) return;

    std::shared_ptr<ShapeModel> shapeModel;
    if (shape_binder_) {
      shapeModel = shape_binder_->model_for(old_item);
    }
    if (!shapeModel) {
      return;
    }

    auto to_shape_type = [](const QString &type) {
      if (type == "circle") return ShapeModel::ShapeType::Circle;
      if (type == "ellipse") return ShapeModel::ShapeType::Ellipse;
      if (type == "stick") return ShapeModel::ShapeType::Stick;
      return ShapeModel::ShapeType::Rectangle;
    };

    ShapeModel::ShapeType targetType = to_shape_type(new_type);
    if (shapeModel->type() == targetType) {
      return;
    }
    shapeModel->set_type(targetType);

    QPointF position = old_graphics_item->pos();
    qreal rotation = old_graphics_item->rotation();
    QString item_name = old_item->name();
    QSizeF old_size = old_graphics_item->boundingRect().size();

    auto update_model_size = [&](ShapeModel::ShapeType type){
      switch (type) {
        case ShapeModel::ShapeType::Circle: {
          double diameter = std::max(old_size.width(), old_size.height());
          if (diameter < 1.0) diameter = 80.0;
          shapeModel->set_size(Size2D{diameter, diameter});
          break;
        }
        case ShapeModel::ShapeType::Rectangle:
        case ShapeModel::ShapeType::Ellipse: {
          QSizeF sz = old_size;
          if (sz.width() < 1.0) sz.setWidth(100.0);
          if (sz.height() < 1.0) sz.setHeight(60.0);
          shapeModel->set_size(Size2D{sz.width(), sz.height()});
          break;
        }
        case ShapeModel::ShapeType::Stick: {
          double length = std::max(old_size.width(), old_size.height());
          if (length < 1.0) length = 100.0;
          constexpr double kDefaultThickness = 2.0;
          shapeModel->set_size(Size2D{length, kDefaultThickness});
          break;
        }
      }
    };
    update_model_size(targetType);

    std::unique_ptr<ISceneObject> new_item_ptr(create_item_for_shape(shapeModel));
    if (!new_item_ptr) return;
    auto *new_item = new_item_ptr.release();
    auto *new_graphics_item = dynamic_cast<QGraphicsItem*>(new_item);
    if (!new_graphics_item) {
      delete new_item;
      return;
    }

    new_item->set_name(item_name);
    new_graphics_item->setPos(position);
    new_graphics_item->setRotation(rotation);
    scene->addItem(new_graphics_item);

    if (shape_binder_) {
      shape_binder_->attach_shape(new_item, shapeModel);
      shape_binder_->unbind_shape(old_item);
    }

    scene->removeItem(old_graphics_item);
    delete old_graphics_item;

    current_selected_item_ = new_graphics_item;
    if (properties_bar_) {
      properties_bar_->set_selected_item(new_item, item_name);
    }
  });
  // Bind model to the Objects bar (first sidebar entry)
  // We know SideBarWidget registered the "objects" page as index 0
  // so we can safely find the first page's widget and cast to ObjectsBar
  // Alternatively SideBarWidget could expose a getter; for now we search children
  // Connect model dataChanged to PropertiesBar update
  connect(tree_model_, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &, const QVector<int> &roles){
    if (roles.contains(Qt::DisplayRole) || roles.isEmpty()) {
      auto shapeModel = tree_model_->shape_from_index(topLeft);
      if (shapeModel && shape_binder_) {
        auto *item = shape_binder_->item_for(shapeModel);
        if (item && item == current_selected_item_) {
          QString name = QString::fromStdString(shapeModel->name());
          properties_bar_->update_name(name);
        }
      }
      // Check if it's a material node
      auto material = tree_model_->material_from_index(topLeft);
      if (material && properties_bar_) {
        // Could refresh material view if needed
      }
    }
  });

  if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
    objectsBar->set_model(tree_model_);
    auto *treeView = objectsBar->treeView();
    if (treeView) {
      // Tree -> Scene selection
      connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
              [this](const QModelIndex &current, const QModelIndex &){
                if (!editor_area_) return;
                auto shapeModel = tree_model_->shape_from_index(current);
                if (shapeModel && shape_binder_) {
                  auto *item = shape_binder_->item_for(shapeModel);
                  if (item) {
                    auto *scene = editor_area_->scene();
                    if (!scene) return;
                    scene->clearSelection();
                    item->setSelected(true);
                    current_selected_item_ = item;
                    if (properties_bar_) {
                      if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
                        QString name = QString::fromStdString(shapeModel->name());
                        properties_bar_->set_selected_item(sceneObj, name);
                      }
                    }
                    return;
                  }
                }
                // Check if it's a material
                auto material = tree_model_->material_from_index(current);
                if (material && properties_bar_) {
                  current_selected_item_ = nullptr;
                  properties_bar_->set_selected_material(material.get());
                }
              });
      // Scene -> Tree selection
      if (auto *scene = editor_area_->scene()) {
        connect(scene, &QGraphicsScene::selectionChanged, this, [this, treeView]{
          auto items = editor_area_->scene()->selectedItems();
          if (items.isEmpty()) {
            // Show substrate when nothing selected
            if (properties_bar_ && editor_area_->substrate_item()) {
              properties_bar_->set_selected_item(editor_area_->substrate_item(), "Substrate");
            }
            return;
          }
          if (shape_binder_) {
            if (auto model = shape_binder_->model_for(dynamic_cast<ISceneObject*>(items.first()))) {
              QModelIndex idx = tree_model_->index_from_shape(model);
              if (idx.isValid()) {
                treeView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
              }
            }
          }
          // Update properties bar
          if (properties_bar_) {
            if (auto *sceneObj = dynamic_cast<ISceneObject*>(items.first())) {
              current_selected_item_ = items.first();
              QString name = sceneObj->name();
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
  sizes << side_bar_widget_->width() << 1200;
  splitter->setSizes(sizes);

  setCentralWidget(splitter);
}

void MainWindow::new_project() {
  // Clear all objects except substrate
  if (!editor_area_) {
    return;
  }
  auto *scene = editor_area_->scene();
  if (!scene) {
    return;
  }

  // Clear properties bar FIRST before deleting items
  if (properties_bar_) {
    properties_bar_->clear();
  }
  current_selected_item_ = nullptr;

  if (document_model_) {
    document_model_->clear_shapes();
    document_model_->clear_materials();
    auto substrate = std::make_shared<SubstrateModel>(Size2D{1000.0, 1000.0}, Color{240, 240, 240, 255});
    substrate->set_name("Substrate");
    document_model_->set_substrate(substrate);
  }

  rebuild_scene_from_document();

  // Clear current file and selection
  current_file_path_.clear();
  current_selected_item_ = nullptr;

  // Show substrate in properties
  if (properties_bar_ && editor_area_->substrate_item()) {
    properties_bar_->set_selected_item(editor_area_->substrate_item(), "Substrate");
  }

  statusBar()->showMessage("New project created", 3000);
}

void MainWindow::save_project() {
  if (current_file_path_.isEmpty()) {
    save_project_as();
    return;
  }

  sync_document_from_scene();
  if (ProjectSerializer::save_to_file(current_file_path_, document_model_.get())) {
    statusBar()->showMessage("Project saved successfully", 3000);
  } else {
    statusBar()->showMessage("Failed to save project", 3000);
  }
}

void MainWindow::save_project_as() {
  QSettings settings("NIR", "MaterialEditor");
  QString lastDir = settings.value("lastDirectory", QDir::homePath()).toString();
  QString defaultPath = lastDir + "/untitled.json";

  QString filename = QFileDialog::getSaveFileName(
      this,
      "Save Project As",
      defaultPath,
      "JSON Files (*.json)",
      nullptr,
      QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty()) {
    return;
  }

  sync_document_from_scene();
  if (ProjectSerializer::save_to_file(filename, document_model_.get())) {
    current_file_path_ = filename;
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project saved successfully", 3000);
  } else {
    statusBar()->showMessage("Failed to save project", 3000);
  }
}

void MainWindow::open_project() {
  QSettings settings("NIR", "MaterialEditor");
  QString lastDir = settings.value("lastDirectory", QDir::homePath()).toString();

  QString filename = QFileDialog::getOpenFileName(
      this,
      "Open Project",
      lastDir,
      "JSON Files (*.json)",
      nullptr,
      QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty()) {
    return;
  }

  // Clear properties bar before loading to avoid accessing deleted items
  if (properties_bar_) {
    properties_bar_->clear();
  }
  current_selected_item_ = nullptr;

  if (ProjectSerializer::load_from_file(filename, document_model_.get())) {
    rebuild_scene_from_document();
    current_file_path_ = filename;
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project loaded successfully", 3000);
  } else {
    statusBar()->showMessage("Failed to load project", 3000);
  }
}

void MainWindow::sync_document_from_scene() {
  if (!document_model_ || !editor_area_) {
    return;
  }
  auto substrate = document_model_->substrate();
  if (!substrate) {
    substrate = std::make_shared<SubstrateModel>();
    document_model_->set_substrate(substrate);
  }
  if (auto *substrate_item = editor_area_->substrate_item()) {
    const QSizeF size = substrate_item->size();
    substrate->set_size(Size2D{size.width(), size.height()});
    substrate->set_color(to_model_color(substrate_item->fill_color()));
    substrate->set_name(substrate_item->name().toStdString());
  }
}

ISceneObject* MainWindow::create_item_for_shape(const std::shared_ptr<ShapeModel> &shape) {
  if (!shape) {
    return nullptr;
  }
  const Size2D size = shape->size();
  switch (shape->type()) {
    case ShapeModel::ShapeType::Rectangle: {
      auto *item = new RectangleItem(QRectF(0, 0, size.width, size.height));
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Ellipse: {
      auto *item = new EllipseItem(QRectF(0, 0, size.width, size.height));
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Circle: {
      const qreal radius = static_cast<qreal>(size.width) / 2.0;
      auto *item = new CircleItem(radius > 0 ? radius : 50.0);
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
    case ShapeModel::ShapeType::Stick: {
      const qreal halfLen = static_cast<qreal>(size.width) / 2.0;
      auto *item = new StickItem(QLineF(-halfLen, 0.0, halfLen, 0.0));
      QPen pen = item->pen();
      double thickness = size.height > 0 ? size.height : pen.widthF();
      pen.setWidthF(std::clamp(thickness, 2.0, 6.0));
      item->setPen(pen);
      item->set_name(QString::fromStdString(shape->name()));
      return item;
    }
  }
  return nullptr;
}

void MainWindow::rebuild_scene_from_document() {
  if (!document_model_ || !editor_area_ || !shape_binder_) {
    return;
  }
  auto *scene = editor_area_->scene();
  if (!scene) {
    return;
  }

  if (properties_bar_) {
    properties_bar_->clear();
  }
  current_selected_item_ = nullptr;

  shape_binder_->clear_bindings();

  QList<QGraphicsItem*> toRemove;
  for (QGraphicsItem *item : scene->items()) {
    if (dynamic_cast<SubstrateItem*>(item) == nullptr) {
      toRemove.append(item);
    }
  }
  for (QGraphicsItem *item : toRemove) {
    scene->removeItem(item);
    delete item;
  }

  if (auto substrate_model = document_model_->substrate()) {
    if (auto *substrate_item = editor_area_->substrate_item()) {
      substrate_item->set_size(QSizeF(substrate_model->size().width, substrate_model->size().height));
      substrate_item->set_fill_color(to_qcolor(substrate_model->color()));
      substrate_item->set_name(QString::fromStdString(substrate_model->name()));
    }
  }

  for (const auto &shape : document_model_->shapes()) {
    if (!shape) {
      continue;
    }
    if (auto *sceneObj = create_item_for_shape(shape)) {
      if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(sceneObj)) {
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

