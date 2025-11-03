#pragma once

#include <QGraphicsView>
#include <memory>

class QGraphicsScene;
class QGraphicsItem;

class EditorView : public QGraphicsView {
  Q_OBJECT
public:
  explicit EditorView(QWidget *parent = nullptr);
  ~EditorView() override;

  void fitToItem(const QGraphicsItem *item);

private:
  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

  void applyZoom(qreal factor);

private:
  std::unique_ptr<QGraphicsScene> scene_;
  bool panning_ {false};
  QPoint last_mouse_pos_;
  qreal scale_ {1.0};
};
