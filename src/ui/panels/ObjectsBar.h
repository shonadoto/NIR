#pragma once

#include <QAbstractItemModel>
#include <QWidget>

class QTreeView;
class QPushButton;
class QToolBar;
class ObjectTreeModel;
class EditorArea;
class ShapeModelBinder;
class CommandManager;
class DocumentModel;

class ObjectsBar : public QWidget {
  Q_OBJECT
 public:
  explicit ObjectsBar(QWidget* parent = nullptr);
  ~ObjectsBar() override;

  QTreeView* treeView() const {
    return tree_view_;
  }
  int preferredWidth() const {
    return preferred_width_;
  }
  void setPreferredWidth(int w);
  void set_model(QAbstractItemModel* model);
  void set_editor_area(EditorArea* editor_area);
  void set_shape_binder(ShapeModelBinder* binder);
  void set_command_manager(CommandManager* command_manager);
  void set_document_model(DocumentModel* document_model);

 private slots:
  void add_item_or_preset();
  void remove_selected_item();

 private:
  bool eventFilter(QObject* obj, QEvent* event) override;
  void setup_toolbar();
  void update_button_states();

 public slots:
  void setActive(bool visible) {
    setVisible(visible);
  }
  void toggle() {
    setVisible(!isVisible());
  }

 private:
  QTreeView* tree_view_{nullptr};
  QToolBar* toolbar_{nullptr};
  QPushButton* add_btn_{nullptr};
  QPushButton* remove_btn_{nullptr};
  EditorArea* editor_area_{nullptr};
  int preferred_width_;
  int last_visible_width_;
  ShapeModelBinder* shape_binder_{nullptr};
  CommandManager* command_manager_{nullptr};
  DocumentModel* document_model_{nullptr};

 protected:
  void hideEvent(QHideEvent* event) override;
  void showEvent(QShowEvent* event) override;
};
