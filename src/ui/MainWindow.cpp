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
#include "model/ObjectTreeModel.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>

namespace {
constexpr int kDefaultObjectsBarWidthPx = 280;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("NIR Material Editor");
  setWindowIcon(QIcon(":/icons/app.svg"));

  createActivityObjectsBarAndEditor();
  createActionsAndToolbar();

  resize(1200, 800);
}

MainWindow::~MainWindow() = default;

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
    auto *rect = new QGraphicsRectItem(QRectF(-50, -30, 100, 60));
    rect->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    scene->addItem(rect);
    rect->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(rect, "Rectangle");
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
    auto *ellipse = new QGraphicsEllipseItem(QRectF(-50, -30, 100, 60));
    ellipse->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    scene->addItem(ellipse);
    ellipse->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(ellipse, "Ellipse");
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
    auto *circle = new QGraphicsEllipseItem(QRectF(-40, -40, 80, 80));
    circle->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    scene->addItem(circle);
    circle->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(circle, "Circle");
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
    auto *stick = new QGraphicsLineItem(QLineF(-50, 0, 50, 0));
    stick->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    QPen pen(Qt::black);
    pen.setWidthF(2.0);
    stick->setPen(pen);
    scene->addItem(stick);
    stick->setPos(c);
    if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
      if (auto *model = qobject_cast<ObjectTreeModel*>(objectsBar->treeView()->model())) {
        model->add_item(stick, "Stick");
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

  // Right: Editor area
  editor_area_ = new EditorArea(splitter);

  // Object tree model (root + substrate)
  auto *treeModel = new ObjectTreeModel(this);
  treeModel->set_substrate(editor_area_->substrate_item());
  // Bind model to the Objects bar (first sidebar entry)
  // We know SideBarWidget registered the "objects" page as index 0
  // so we can safely find the first page's widget and cast to ObjectsBar
  // Alternatively SideBarWidget could expose a getter; for now we search children
  if (auto *objectsBar = side_bar_widget_->findChild<ObjectsBar*>()) {
    objectsBar->set_model(treeModel);
    auto *treeView = objectsBar->treeView();
    if (treeView) {
      // Tree -> Scene selection
      connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
              [this, treeModel](const QModelIndex &current, const QModelIndex &){
                if (!editor_area_) return;
                auto *item = treeModel->item_from_index(current);
                if (!item) return;
                auto *scene = editor_area_->scene();
                if (!scene) return;
                scene->clearSelection();
                item->setSelected(true);
              });
      // Scene -> Tree selection
      if (auto *scene = editor_area_->scene()) {
        connect(scene, &QGraphicsScene::selectionChanged, this, [this, treeModel, treeView]{
          auto items = editor_area_->scene()->selectedItems();
          if (items.isEmpty()) return;
          QModelIndex idx = treeModel->index_from_item(items.first());
          if (idx.isValid()) {
            treeView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
          }
        });
      }
    }
  }

  // Put into splitter
  splitter->addWidget(side_bar_widget_);
  splitter->addWidget(editor_area_);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  QList<int> sizes;
  sizes << side_bar_widget_->width() << 1200;
  splitter->setSizes(sizes);

  setCentralWidget(splitter);
}

