#pragma once

#include <QWidget>

class QVBoxLayout;
class QLabel;
class ISceneObject;

class PropertiesBar : public QWidget {
  Q_OBJECT
public:
  explicit PropertiesBar(QWidget *parent = nullptr);
  ~PropertiesBar() override;

  void set_selected_item(ISceneObject *item);
  void clear();

private:
  QVBoxLayout *layout_ {nullptr};
  QLabel *title_ {nullptr};
  QWidget *content_widget_ {nullptr};
  ISceneObject *current_item_ {nullptr};
  int preferred_width_ {280};
};
