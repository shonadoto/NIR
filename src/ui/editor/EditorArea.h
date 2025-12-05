#pragma once

#include <QWidget>

class EditorView;
class SubstrateItem;
class QGraphicsScene;

class EditorArea : public QWidget {
  Q_OBJECT
 public:
  explicit EditorArea(QWidget* parent = nullptr);
  ~EditorArea() override;

  EditorView* view() const {
    return view_;
  }
  void fit_to_substrate();
  void set_substrate_size(const QSizeF& size);
  auto substrate_size() const -> QSizeF;
  SubstrateItem* substrate_item() const {
    return substrate_;
  }
  auto scene() const -> QGraphicsScene*;
  auto substrate_center() const -> QPointF;

 private:
  void init_scene();
  void showEvent(QShowEvent* event) override;

 private:
  EditorView* view_{nullptr};
  QGraphicsScene* scene_{nullptr};
  SubstrateItem* substrate_{nullptr};
  bool fitted_once_{false};
};
