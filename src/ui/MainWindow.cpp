#include "MainWindow.h"

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QSize>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include "EditorView.h"
#include "PanelArea.h"

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
}

void MainWindow::createActivityObjectsBarAndEditor() {
  auto *root = new QWidget(this);
  auto *rootLayout = new QHBoxLayout(root);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  leftContainer_ = new QWidget(root);
  auto *leftLayout = new QHBoxLayout(leftContainer_);
  leftLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->setSpacing(0);

  activityBar_ = new QWidget(leftContainer_);
  activityBar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  auto *barLayout = new QVBoxLayout(activityBar_);
  const int barMargin = 6; // equal left/right offset around the button
  barLayout->setContentsMargins(barMargin, barMargin, barMargin, barMargin);
  barLayout->setSpacing(barMargin);

  QIcon objectsIcon(":/icons/objects.svg");
  objectsButton_ = new QToolButton(activityBar_);
  objectsButton_->setCheckable(true);
  objectsButton_->setChecked(true);
  objectsButton_->setIcon(objectsIcon);
  objectsButton_->setIconSize(QSize(24, 24));
  objectsButton_->setFixedSize(32, 32);
  objectsButton_->setAutoRaise(true);
  barLayout->addWidget(objectsButton_, 0, Qt::AlignTop);
  connect(objectsButton_, &QToolButton::toggled, this,
          &MainWindow::showObjectsBar);

  activityBarFixedWidth_ =
      objectsButton_->sizeHint().width() + 2 * barMargin; // 32 + 2*m
  activityBar_->setFixedWidth(activityBarFixedWidth_);

  panelArea_ = new PanelArea(root);

  // Objects panel (left of editor within the grid)
  objectsPanel_ = new QWidget(panelArea_);
  auto *objectsLayout = new QVBoxLayout(objectsPanel_);
  objectsLayout->setContentsMargins(0, 0, 0, 0);
  objectTreeView_ = new QTreeView(objectsPanel_);
  objectsLayout->addWidget(objectTreeView_);
  objectsPanel_->setMinimumWidth(220);
  objectsPanel_->setFixedWidth(lastObjectsBarWidth_);

  leftLayout->addWidget(activityBar_);
  rootLayout->addWidget(leftContainer_);

  // Add panels to the grid: column 0 - objects, column 1 - editor
  panelArea_->addPanel(objectsPanel_, 0, 0);
  editorView_ = new EditorView(panelArea_);
  panelArea_->addPanel(editorView_, 0, 1);
  panelArea_->setColumnStretch(1, 1);
  panelArea_->setRowStretch(0, 1);

  // place objects panel into grid column 0 (optional: could be in
  // leftContainer_ only)
  panelArea_->addPanel(new QWidget(panelArea_), 1,
                       1); // placeholder for future expansion

  // Put panel area to the right of activity bar + objects panel container
  rootLayout->addWidget(panelArea_, 1);

  setCentralWidget(root);
}

void MainWindow::toggleObjectsBar() {
  if (objectsBarVisible_) {
    if (objectsPanel_) {
      lastObjectsBarWidth_ = objectsPanel_->width();
      objectsPanel_->setVisible(false);
    }
    objectsBarVisible_ = false;
  } else {
    objectsBarVisible_ = true;
    if (objectsPanel_) {
      objectsPanel_->setVisible(true);
      objectsPanel_->setFixedWidth(
          lastObjectsBarWidth_ > 0 ? lastObjectsBarWidth_ : 280);
    }
  }
}

void MainWindow::activateObjectsPanel() {
  if (!objectsBarVisible_) {
    showObjectsBar(true);
  }
}

void MainWindow::showObjectsBar(bool show) {
  if (show == objectsBarVisible_)
    return;
  toggleObjectsBar();
}
