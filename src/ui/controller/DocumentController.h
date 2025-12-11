#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "model/ShapeModel.h"
#include "model/core/ModelTypes.h"

class DocumentModel;
class ShapeModelBinder;
class EditorArea;
class ISceneObject;
class QGraphicsItem;
class QGraphicsScene;
class SubstrateItem;
class ShapeModel;
class CommandManager;

/**
 * @brief Controller for managing document operations and synchronization
 * between DocumentModel and Scene.
 *
 * This class handles:
 * - Document lifecycle (new, save, load)
 * - Synchronization between model and scene
 * - Shape creation and management
 * - Scene rebuilding from document
 */
class DocumentController : public QObject {
  Q_OBJECT

 public:
  explicit DocumentController(QObject* parent = nullptr);
  ~DocumentController() override;

  void set_document_model(DocumentModel* document);
  void set_shape_binder(ShapeModelBinder* binder);
  void set_editor_area(EditorArea* editor_area);
  void set_command_manager(CommandManager* command_manager);

  // Document operations
  void new_document();
  bool save_document(const QString& file_path);
  bool load_document(const QString& file_path);
  QString current_file_path() const {
    return current_file_path_;
  }
  void set_current_file_path(const QString& path) {
    current_file_path_ = path;
  }

  // Scene synchronization
  void rebuild_scene_from_document();
  void sync_document_from_scene();

  // Shape operations
  static auto create_item_for_shape(const std::shared_ptr<ShapeModel>& shape)
    -> ISceneObject*;
  void change_shape_type(ISceneObject* old_item, const QString& new_type);
  void replace_shape_item(ISceneObject* old_item,
                          const std::shared_ptr<ShapeModel>& model,
                          const QPointF& center_position, qreal rotation,
                          const QString& name);

  // Utility methods
  static auto string_to_shape_type(const QString& type)
    -> ShapeModel::ShapeType;
  static auto convert_shape_size(ShapeModel::ShapeType from,
                                 ShapeModel::ShapeType target_type,
                                 const Size2D& size) -> Size2D;

  // Getters
  DocumentModel* document_model() const {
    return document_model_;
  }
  ShapeModelBinder* shape_binder() const {
    return shape_binder_;
  }
  EditorArea* editor_area() const {
    return editor_area_;
  }
  CommandManager* command_manager() const {
    return command_manager_;
  }

 signals:
  void document_changed();
  void file_path_changed(const QString& path);

 private:
  void clear_scene_except_substrate();
  void update_substrate_from_model();
  void create_shapes_in_scene();

  DocumentModel* document_model_{nullptr};
  ShapeModelBinder* shape_binder_{nullptr};
  EditorArea* editor_area_{nullptr};
  CommandManager* command_manager_{nullptr};
  QString current_file_path_;
};
