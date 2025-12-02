#pragma once

#include <QWidget>
#include <memory>

class QVBoxLayout;
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QDoubleSpinBox;
class ISceneObject;
class MaterialModel;
class ObjectTreeModel;
class ShapeModelBinder;
class ShapeModel;

class PropertiesBar : public QWidget {
  Q_OBJECT
public:
  explicit PropertiesBar(QWidget *parent = nullptr);
  ~PropertiesBar() override;

  void set_selected_item(ISceneObject *item, const QString &name);
  void set_selected_material(MaterialModel *material);
  void set_model(ObjectTreeModel *model);
  void set_shape_binder(ShapeModelBinder *binder) { shape_binder_ = binder; }
  void clear();
  void update_name(const QString &name);

signals:
  void name_changed(const QString &new_name);
  void type_changed(ISceneObject *item, const QString &new_type);
  void material_name_changed(MaterialModel *material, const QString &new_name);
  void material_color_changed(MaterialModel *material, const QColor &color);
  void item_material_changed(ISceneObject *item, MaterialModel *material);

private:
  void setup_type_selector();
  void setup_material_selector();
  void setup_grid_controls();
  bool is_inclusion_item() const;
  void update_material_ui();
  void update_material_color_button();
  void update_grid_controls();
  bool can_edit_material_color() const;

  QVBoxLayout *layout_ {nullptr};
  QLabel *type_label_ {nullptr};
  QLineEdit *name_edit_ {nullptr};
  QComboBox *type_combo_ {nullptr};
  QComboBox *material_combo_ {nullptr};
  QPushButton *material_color_btn_ {nullptr};
  QLabel *grid_type_label_ {nullptr};
  QComboBox *grid_type_combo_ {nullptr};
  QDoubleSpinBox *grid_frequency_x_spin_ {nullptr};
  QDoubleSpinBox *grid_frequency_y_spin_ {nullptr};
  QWidget *content_widget_ {nullptr};
  ISceneObject *current_item_ {nullptr};
  MaterialModel *current_material_ {nullptr};
  ObjectTreeModel *model_ {nullptr};
  MaterialModel *item_material_ {nullptr}; // Material assigned to current item (nullptr = Custom)
  int preferred_width_ {280};
  bool updating_ {false};
  ShapeModelBinder *shape_binder_ {nullptr};
  std::shared_ptr<ShapeModel> current_model_;

  // Connection IDs for model change signals
  int material_connection_id_ {0};
  int shape_model_connection_id_ {0};
  std::shared_ptr<MaterialModel> current_material_shared_; // Keep shared_ptr to prevent deletion

  void disconnect_model_signals();
  void connect_model_signals();
  std::shared_ptr<MaterialModel> find_material(MaterialModel *raw) const;
};
