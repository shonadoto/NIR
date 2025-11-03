#include "MainWindow.h"

#include <QAction>
#include <QDockWidget>
#include <QIcon>
#include <QToolBar>
#include <QTreeView>

#include "EditorView.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("NIR Material Editor");
    setWindowIcon(QIcon(":/icons/app.svg"));

    createUi();
    createActionsAndToolbar();
    createDockWidgets();

    resize(1200, 800);
}

MainWindow::~MainWindow() = default;

void MainWindow::createUi() {
    editorView_ = new EditorView(this);
    setCentralWidget(editorView_);
}

void MainWindow::createActionsAndToolbar() {
    auto *toolbar = addToolBar("Tools");
    toolbar->setMovable(true);

    // Placeholders for future tools
    auto *selectAction = new QAction("Select", this);
    auto *addRectAction = new QAction("Rect", this);
    auto *addEllipseAction = new QAction("Ellipse", this);
    auto *addCircleAction = new QAction("Circle", this);
    auto *addStickAction = new QAction("Stick", this);

    toolbar->addAction(selectAction);
    toolbar->addSeparator();
    toolbar->addAction(addRectAction);
    toolbar->addAction(addEllipseAction);
    toolbar->addAction(addCircleAction);
    toolbar->addAction(addStickAction);
}

void MainWindow::createDockWidgets() {
    objectTreeView_ = new QTreeView(this);
    auto *objectsDock = new QDockWidget("Objects", this);
    objectsDock->setObjectName("ObjectsDock");
    objectsDock->setWidget(objectTreeView_);
    addDockWidget(Qt::LeftDockWidgetArea, objectsDock);
}


