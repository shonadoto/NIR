#pragma once

#include <QMainWindow>
#include <memory>
#include <QString>
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
class DocumentModel;
class ShapeModelBinder;

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
    void rebuild_scene_from_document();
    void sync_document_from_scene();
    ISceneObject* create_item_for_shape(const std::shared_ptr<ShapeModel> &shape);
    
    // Shape type change handling
    void change_shape_type(ISceneObject *old_item, const QString &new_type);
    ShapeModel::ShapeType string_to_shape_type(const QString &type) const;
    Size2D convert_shape_size(ShapeModel::ShapeType from, ShapeModel::ShapeType to, const Size2D &size) const;
    void replace_shape_item(ISceneObject *old_item, const std::shared_ptr<ShapeModel> &model, 
                            const QPointF &centerPosition, qreal rotation, const QString &name);


private:
    SideBarWidget *side_bar_widget_ {nullptr};
    EditorArea *editor_area_ {nullptr};
    PropertiesBar *properties_bar_ {nullptr};
    ObjectTreeModel *tree_model_ {nullptr};
    QString current_file_path_;
    QGraphicsItem *current_selected_item_ {nullptr};
    std::unique_ptr<DocumentModel> document_model_;
    std::unique_ptr<ShapeModelBinder> shape_binder_;
};
