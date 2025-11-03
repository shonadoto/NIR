#include "EditorView.h"

#include <QGraphicsScene>
#include <QPainter>

EditorView::EditorView(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)) {
  setScene(scene_);
  setRenderHint(QPainter::Antialiasing, true);
  setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  setDragMode(QGraphicsView::NoDrag);

  // Initial scene rect; will be adjusted in later stages when substrate is
  // added
  scene_->setSceneRect(-500, -500, 1000, 1000);
}

EditorView::~EditorView() = default;
