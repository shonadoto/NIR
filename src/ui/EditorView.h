#pragma once

#include <QGraphicsView>

class QGraphicsScene;

class EditorView : public QGraphicsView {
  Q_OBJECT
public:
  explicit EditorView(QWidget *parent = nullptr);
  ~EditorView() override;

private:
  QGraphicsScene *scene_{nullptr};
};
