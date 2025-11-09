#pragma once

#include <QMainWindow>

class QTreeView;
class QToolBar;
class QWidget;
class SideBarWidget;
class ObjectsBar;
class EditorArea;
class PropertiesBar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createActionsAndToolbar();
    void createActivityObjectsBarAndEditor();


private:
    SideBarWidget *side_bar_widget_ {nullptr};
    EditorArea *editor_area_ {nullptr};
    PropertiesBar *properties_bar_ {nullptr};
};
