#pragma once

#include <QWidget>

class QVBoxLayout;
class QLabel;
class QLineEdit;
class ISceneObject;

class PropertiesBar : public QWidget {
  Q_OBJECT
public:
  explicit PropertiesBar(QWidget *parent = nullptr);
  ~PropertiesBar() override;

  void set_selected_item(ISceneObject *item, const QString &name);
  void clear();
  void update_name(const QString &name);

signals:
  void name_changed(const QString &new_name);

private:
  QVBoxLayout *layout_ {nullptr};
  QLabel *type_label_ {nullptr};
  QLineEdit *name_edit_ {nullptr};
  QWidget *content_widget_ {nullptr};
  ISceneObject *current_item_ {nullptr};
  int preferred_width_ {280};
  bool updating_ {false};
};
