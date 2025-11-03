#include "EditorView.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QtMath>
#include <algorithm>

namespace {
constexpr qreal kZoomStep = 1.15;       // per mouse wheel notch
constexpr qreal kMinScale = 0.05;       // 5%
constexpr qreal kMaxScale = 10.0;       // 1000%
}

EditorView::EditorView(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)) {
  setScene(scene_);
  setRenderHint(QPainter::Antialiasing, true);
  setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  setDragMode(QGraphicsView::NoDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setAlignment(Qt::AlignCenter);
}

EditorView::~EditorView() = default;

void EditorView::fitToItem(const QGraphicsItem *item) {
  if (item == nullptr) {
    return;
  }
  const QRectF br = item->sceneBoundingRect();
  if (br.isEmpty()) {
    return;
  }
  fitInView(br, Qt::KeepAspectRatio);
  centerOn(item);
  // Update scale_ to reflect current transform approximately
  scale_ = transform().m11();
}

void EditorView::wheelEvent(QWheelEvent *event) {
  const QPoint num_degrees = event->angleDelta() / 8;
  if (num_degrees.y() == 0) {
    QGraphicsView::wheelEvent(event);
    return;
  }

  const int num_steps = num_degrees.y() / 15; // 15 deg per notch
  const qreal factor = (num_steps > 0) ? std::pow(kZoomStep, num_steps)
                                       : std::pow(1.0 / kZoomStep, -num_steps);
  applyZoom(factor);
}

void EditorView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::MiddleButton ||
      (event->button() == Qt::LeftButton && (event->modifiers() & Qt::KeyboardModifier::ShiftModifier))) {
    panning_ = true;
    last_mouse_pos_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
    return;
  }
  QGraphicsView::mousePressEvent(event);
}

void EditorView::mouseMoveEvent(QMouseEvent *event) {
  if (panning_) {
    const QPoint delta = event->pos() - last_mouse_pos_;
    last_mouse_pos_ = event->pos();
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    event->accept();
    return;
  }
  QGraphicsView::mouseMoveEvent(event);
}

void EditorView::mouseReleaseEvent(QMouseEvent *event) {
  if (panning_ && (event->button() == Qt::MiddleButton ||
                   (event->button() == Qt::LeftButton && (event->modifiers() & Qt::KeyboardModifier::ShiftModifier)))) {
    panning_ = false;
    setCursor(Qt::ArrowCursor);
    event->accept();
    return;
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void EditorView::applyZoom(qreal factor) {
  const qreal new_scale = std::clamp(scale_ * factor, kMinScale, kMaxScale);
  factor = new_scale / scale_;
  scale_ = new_scale;
  scale(factor, factor);
}
