#pragma once

#include <QMainWindow>

class QTreeView;
class QDockWidget;
class QToolBar;
class EditorView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createUi();
    void createActionsAndToolbar();
    void createDockWidgets();

private:
    EditorView *editorView_ {nullptr};
    QTreeView *objectTreeView_ {nullptr};
};


