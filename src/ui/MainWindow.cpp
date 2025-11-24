#include "MainWindow.h"

#include <QAction>
#include <QIcon>
#include <QSize>
#include <QSplitter>
#include <QToolBar>
#include <QTreeView>
#include <QGraphicsScene>
#include <QItemSelectionModel>
#include "ui/editor/EditorArea.h"
#include "ui/panels/ObjectsBar.h"
#include "ui/sidebar/SideBarWidget.h"
#include "ui/editor/SubstrateDialog.h"
#include "ui/editor/SubstrateItem.h"
#include "ui/panels/PropertiesBar.h"
#include "model/ObjectTreeModel.h"
#include "model/MaterialPreset.h"
#include "scene/ISceneObject.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/CircleItem.h"
#include "scene/items/StickItem.h"
#include "serialization/ProjectSerializer.h"
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

  createActivityObjectsBarAndEditor();
  createMenuBar();
  createActionsAndToolbar();

  resize(1200, 800);
}

MainWindow::~MainWindow() {
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
  side_bar_widget_->registerSidebar(
      "objects", QIcon(":/icons/objects.svg"),
      objectsBar, kDefaultObjectsBarWidthPx);

  // Middle/Right: Editor area + Properties bar
  auto *rightSplitter = new QSplitter(Qt::Horizontal, splitter);
  rightSplitter->setChildrenCollapsible(false);
  rightSplitter->setHandleWidth(1);

  editor_area_ = new EditorArea(rightSplitter);
  properties_bar_ = new PropertiesBar(rightSplitter);
  // Always visible, show substrate by default

  rightSplitter->addWidget(editor_area_);
  rightSplitter->addWidget(properties_bar_);
  rightSplitter->setStretchFactor(0, 1);
  rightSplitter->setStretchFactor(1, 0);

  // Object tree model (root + substrate)
  tree_model_ = new ObjectTreeModel(this);
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
    QModelIndex idx = tree_model_->index_from_item(current_selected_item_);
    if (idx.isValid()) {
      tree_model_->setData(idx, new_name, Qt::EditRole);
    }
  });

  // Connect PropertiesBar preset name change to model update
  connect(properties_bar_, &PropertiesBar::preset_name_changed, this, [this](MaterialPreset *preset, const QString &new_name){
    if (!preset || !tree_model_) return;
    // set_name already validates and sets the name
    QModelIndex idx = tree_model_->index_from_preset(preset);
    if (idx.isValid()) {
      tree_model_->setData(idx, new_name, Qt::EditRole);
    }
  });

  // Connect PropertiesBar preset color change
  connect(properties_bar_, &PropertiesBar::preset_color_changed, this, [this](MaterialPreset *preset, const QColor &color){
    if (!preset) return;
    preset->set_fill_color(color);
    // Update all items using this preset
    if (editor_area_ && editor_area_->scene()) {
      // TODO: Update items that use this preset
    }
  });

  // Connect PropertiesBar item material change
  connect(properties_bar_, &PropertiesBar::item_material_changed, this, [this](ISceneObject *item, MaterialPreset *preset){
    if (!item || !editor_area_) return;
    auto *graphicsItem = dynamic_cast<QGraphicsItem*>(item);
    if (!graphicsItem) return;

    QColor color;
    if (preset) {
      // Use preset color
      color = preset->fill_color();
    } else {
      // Custom - keep current color, don't change
      return;
    }

    // Apply color to item
    if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
      rectItem->setBrush(QBrush(color));
      rectItem->update(); // Force repaint
    } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
      ellipseItem->setBrush(QBrush(color));
      ellipseItem->update(); // Force repaint
    } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
      QPen pen = lineItem->pen();
      pen.setColor(color);
      lineItem->setPen(pen);
      lineItem->update(); // Force repaint
    }

    // Update scene to reflect changes
    if (auto *scene = graphicsItem->scene()) {
      scene->update(graphicsItem->boundingRect());
    }
  });

  // Connect PropertiesBar type change to replace object
  connect(properties_bar_, &PropertiesBar::type_changed, this, [this](ISceneObject *old_item, const QString &new_type){
    if (!old_item || !editor_area_ || !tree_model_) return;
    auto *old_graphics_item = dynamic_cast<QGraphicsItem*>(old_item);
    if (!old_graphics_item) return;

    auto *scene = editor_area_->scene();
    if (!scene) return;

    // Get model index BEFORE any operations that might invalidate it
    QModelIndex old_idx = tree_model_->index_from_item(old_graphics_item);
    if (!old_idx.isValid()) return;
    QModelIndex parent = old_idx.parent();
    int row = old_idx.row();

    // Save ALL properties from old item BEFORE deletion
    QString item_name = old_item->name();
    QPointF position = old_graphics_item->pos();
    qreal rotation = old_graphics_item->rotation();
    QColor fill_color;

    // Get fill color based on item type
    if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(old_graphics_item)) {
      fill_color = rectItem->brush().color();
    } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(old_graphics_item)) {
      fill_color = ellipseItem->brush().color();
    } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(old_graphics_item)) {
      fill_color = lineItem->pen().color();
    }

    // Get size/bounds
    QRectF bounds = old_graphics_item->boundingRect();
    QSizeF size(bounds.width(), bounds.height());

    // Create new item based on type
    ISceneObject *new_item = nullptr;
    QGraphicsItem *new_graphics_item = nullptr;

    if (new_type == "circle") {
      qreal radius = qMin(size.width(), size.height()) / 2.0;
      if (radius < 1.0) radius = 40.0;
      auto *circle = new CircleItem(radius);
      scene->addItem(circle);
      new_item = circle;
      new_graphics_item = circle;
    } else if (new_type == "rectangle") {
      if (size.width() < 1.0) size.setWidth(100.0);
      if (size.height() < 1.0) size.setHeight(60.0);
      auto *rect = new RectangleItem(QRectF(-size.width()/2, -size.height()/2, size.width(), size.height()));
      scene->addItem(rect);
      new_item = rect;
      new_graphics_item = rect;
    } else if (new_type == "ellipse") {
      if (size.width() < 1.0) size.setWidth(100.0);
      if (size.height() < 1.0) size.setHeight(60.0);
      auto *ellipse = new EllipseItem(QRectF(-size.width()/2, -size.height()/2, size.width(), size.height()));
      scene->addItem(ellipse);
      new_item = ellipse;
      new_graphics_item = ellipse;
    } else if (new_type == "stick") {
      qreal length = qMax(size.width(), size.height());
      if (length < 1.0) length = 100.0;
      auto *stick = new StickItem(QLineF(-length/2, 0, length/2, 0));
      scene->addItem(stick);
      new_item = stick;
      new_graphics_item = stick;
    }

    if (!new_item || !new_graphics_item) return;

    // Apply saved properties to new item
    new_graphics_item->setPos(position);
    new_graphics_item->setRotation(rotation);
    new_item->set_name(item_name);

    // Apply color
    if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(new_graphics_item)) {
      rectItem->setBrush(QBrush(fill_color));
    } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(new_graphics_item)) {
      ellipseItem->setBrush(QBrush(fill_color));
    } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(new_graphics_item)) {
      QPen pen = lineItem->pen();
      pen.setColor(fill_color);
      lineItem->setPen(pen);
    }

    // Now remove old item from model (this will also remove it from scene and delete it)
    // Note: removeRow will remove from scene and delete the item, so we don't need to do it manually
    tree_model_->removeRow(row, parent);

    // Add new item to model
    tree_model_->add_item(new_graphics_item, item_name);

    // Select new item
    scene->clearSelection();
    new_graphics_item->setSelected(true);
    current_selected_item_ = new_graphics_item;

    // Update properties bar
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
      auto *item = tree_model_->item_from_index(topLeft);
      if (item && item == current_selected_item_) {
        QString name = tree_model_->get_item_name(item);
        properties_bar_->update_name(name);
      } else {
        // Check if it's a preset
        auto *preset = tree_model_->preset_from_index(topLeft);
        if (preset && properties_bar_) {
          // Update preset name in properties bar if it's currently selected
          // Note: We need to check if current_preset_ matches, but PropertiesBar doesn't expose that
          // For now, just update if the preset is selected
        }
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
                // Check if it's an inclusion item
                auto *item = tree_model_->item_from_index(current);
                if (item) {
                  auto *scene = editor_area_->scene();
                  if (!scene) return;
                  scene->clearSelection();
                  item->setSelected(true);
                  // Update properties bar
                  if (properties_bar_) {
                    if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
                      current_selected_item_ = item;
                      QString name = tree_model_->get_item_name(item);
                      properties_bar_->set_selected_item(sceneObj, name);
                    }
                  }
                  return;
                }
                // Check if it's a material preset
                auto *preset = tree_model_->preset_from_index(current);
                if (preset && properties_bar_) {
                  current_selected_item_ = nullptr;
                  properties_bar_->set_selected_preset(preset);
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
          QModelIndex idx = tree_model_->index_from_item(items.first());
          if (idx.isValid()) {
            treeView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
          }
          // Update properties bar
          if (properties_bar_) {
            if (auto *sceneObj = dynamic_cast<ISceneObject*>(items.first())) {
              current_selected_item_ = items.first();
              QString name = tree_model_->get_item_name(items.first());
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

  // Clear model first
  if (tree_model_) {
    tree_model_->clear_items();
  }

  QList<QGraphicsItem*> toRemove;
  for (QGraphicsItem *item : scene->items()) {
    if (dynamic_cast<SubstrateItem*>(item) == nullptr) {
      toRemove.append(item);
    }
  }
  for (auto *item : toRemove) {
    scene->removeItem(item);
    delete item;
  }

  // Reset substrate to default
  if (auto *substrate = editor_area_->substrate_item()) {
    substrate->set_size(QSizeF(1000, 1000));
    substrate->set_fill_color(QColor(240, 240, 240));
    substrate->set_name("Substrate");
  }

  // Rebuild model with substrate
  if (tree_model_) {
    tree_model_->set_substrate(editor_area_->substrate_item());
  }

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

  if (ProjectSerializer::save_to_file(current_file_path_, editor_area_, tree_model_)) {
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

  if (ProjectSerializer::save_to_file(filename, editor_area_, tree_model_)) {
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

  if (ProjectSerializer::load_from_file(filename, editor_area_, tree_model_)) {
    current_file_path_ = filename;
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project loaded successfully", 3000);
  } else {
    statusBar()->showMessage("Failed to load project", 3000);
  }
}

