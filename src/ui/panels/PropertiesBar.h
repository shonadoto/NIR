#pragma once

#include <QWidget>

class QVBoxLayout;
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class ISceneObject;
class MaterialPreset;
class ObjectTreeModel;

class PropertiesBar : public QWidget {
  Q_OBJECT
public:
  explicit PropertiesBar(QWidget *parent = nullptr);
  ~PropertiesBar() override;

  void set_selected_item(ISceneObject *item, const QString &name);
  void set_selected_preset(MaterialPreset *preset);
  void set_model(ObjectTreeModel *model);
  void clear();
  void update_name(const QString &name);

signals:
  void name_changed(const QString &new_name);
  void type_changed(ISceneObject *item, const QString &new_type);
  void preset_name_changed(MaterialPreset *preset, const QString &new_name);
  void preset_color_changed(MaterialPreset *preset, const QColor &color);
  void item_material_changed(ISceneObject *item, MaterialPreset *preset);

private:
  void setup_type_selector();
  void setup_material_selector();
  bool is_inclusion_item() const;
  void update_material_ui();
  void update_material_color_button();

  QVBoxLayout *layout_ {nullptr};
  QLabel *type_label_ {nullptr};
  QLineEdit *name_edit_ {nullptr};
  QComboBox *type_combo_ {nullptr};
  QComboBox *material_combo_ {nullptr};
  QPushButton *material_color_btn_ {nullptr};
  QWidget *content_widget_ {nullptr};
  ISceneObject *current_item_ {nullptr};
  MaterialPreset *current_preset_ {nullptr};
  ObjectTreeModel *model_ {nullptr};
  MaterialPreset *item_material_ {nullptr}; // Material assigned to current item (nullptr = Custom)
  int preferred_width_ {280};
  bool updating_ {false};
};
