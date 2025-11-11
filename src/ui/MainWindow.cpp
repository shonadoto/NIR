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
    if (properties_bar_) {
        properties_bar_->clear();
    }
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

  // Add shapes
  auto *addRect = new QAction("Add Rect", this);
  connect(addRect, &QAction::triggered, this, [this]{
    if (!editor_area_) return;
    auto *scene = editor_area_->scene();
    if (!scene) return;
    const QPointF c = editor_area_->substrate_center();
    auto *rect = new RectangleItem(QRectF(-50, -30, 100, 60));
    scene->addItem(rect);
    rect->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(rect, rect->name());
      }
    }
  });
  toolbar->addAction(addRect);

  auto *addEllipse = new QAction("Add Ellipse", this);
  connect(addEllipse, &QAction::triggered, this, [this]{
    if (!editor_area_) return;
    auto *scene = editor_area_->scene();
    if (!scene) return;
    const QPointF c = editor_area_->substrate_center();
    auto *ellipse = new EllipseItem(QRectF(-50, -30, 100, 60));
    scene->addItem(ellipse);
    ellipse->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(ellipse, ellipse->name());
      }
    }
  });
  toolbar->addAction(addEllipse);

  auto *addCircle = new QAction("Add Circle", this);
  connect(addCircle, &QAction::triggered, this, [this]{
    if (!editor_area_) return;
    auto *scene = editor_area_->scene();
    if (!scene) return;
    const QPointF c = editor_area_->substrate_center();
    auto *circle = new CircleItem(40);
    scene->addItem(circle);
    circle->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(circle, circle->name());
      }
    }
  });
  toolbar->addAction(addCircle);

  auto *addStick = new QAction("Add Stick", this);
  connect(addStick, &QAction::triggered, this, [this]{
    if (!editor_area_) return;
    auto *scene = editor_area_->scene();
    if (!scene) return;
    const QPointF c = editor_area_->substrate_center();
    auto *stick = new StickItem(QLineF(-50, 0, 50, 0));
    scene->addItem(stick);
    stick->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(stick, stick->name());
      }
    }
  });
  toolbar->addAction(addStick);
}

void MainWindow::createActivityObjectsBarAndEditor() {
  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(0);

  // Left: SideBarWidget with activity buttons and stack of bars
  side_bar_widget_ = new SideBarWidget(splitter);
  // Register default Objects bar
  side_bar_widget_->registerSidebar(
      "objects", QIcon(":/icons/objects.svg"),
      new ObjectsBar(side_bar_widget_), kDefaultObjectsBarWidthPx);

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
                auto *item = tree_model_->item_from_index(current);
                if (!item) return;
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

  if (ProjectSerializer::load_from_file(filename, editor_area_, tree_model_)) {
    current_file_path_ = filename;
    settings.setValue("lastDirectory", QFileInfo(filename).absolutePath());
    statusBar()->showMessage("Project loaded successfully", 3000);
  } else {
    statusBar()->showMessage("Failed to load project", 3000);
  }
}

