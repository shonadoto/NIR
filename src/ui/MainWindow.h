#pragma once

#include <QMainWindow>

class QTreeView;
class QToolBar;
class QWidget;
class SideBarWidget;
class ObjectsBar;
class EditorArea;
class PropertiesBar;
class ObjectTreeModel;
class ISceneObject;
class QGraphicsItem;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createMenuBar();
    void createActionsAndToolbar();
    void createActivityObjectsBarAndEditor();
    void new_project();
    void save_project();
    void save_project_as();
    void open_project();


private:
    SideBarWidget *side_bar_widget_ {nullptr};
    EditorArea *editor_area_ {nullptr};
    PropertiesBar *properties_bar_ {nullptr};
    ObjectTreeModel *tree_model_ {nullptr};
    QString current_file_path_;
    QGraphicsItem *current_selected_item_ {nullptr};
};
