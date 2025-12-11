#pragma once

#include <QMainWindow>
#include <QString>
#include <memory>

#include "model/ShapeModel.h"
#include "model/core/ModelTypes.h"

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
class DocumentController;
class DocumentModel;
class ShapeModelBinder;
class CommandManager;

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);
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
  SideBarWidget* side_bar_widget_{nullptr};
  EditorArea* editor_area_{nullptr};
  PropertiesBar* properties_bar_{nullptr};
  ObjectTreeModel* tree_model_{nullptr};
  QGraphicsItem* current_selected_item_{nullptr};
  std::unique_ptr<DocumentModel> document_model_;
  std::unique_ptr<ShapeModelBinder> shape_binder_;
  std::unique_ptr<DocumentController> document_controller_;
  std::unique_ptr<CommandManager> command_manager_;
};
