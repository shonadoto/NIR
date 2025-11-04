#include "EditorView.h"

#include "ui/editor/SubstrateItem.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QtGlobal>
#include <QtMath>
#include <algorithm>
#include <qnamespace.h>

namespace {
constexpr qreal kZoomStep = 1.15;   // per mouse wheel notch (symmetrical)
constexpr qreal kMinScale = 0.70;   // 70%
constexpr qreal kMaxScale = 100.0;  // 10000%
constexpr qreal kRotateStepDeg = 5; // rotation step per notch when Ctrl held
constexpr qreal kScaleStep = 1.05;  // item scale step per notch when Shift held
} // namespace

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
  const auto mods = event->modifiers();

#ifdef Q_OS_MACOS
  const bool rotateMod = mods & Qt::ControlModifier; // Cmd
  const bool scaleMod = mods & Qt::AltModifier;      // Shift
#else
  const bool rotateMod = mods & Qt::ControlModifier; // Ctrl
  const bool scaleMod = mods & Qt::MetaModifier;     // Shift
#endif

  // Transform selected items with modifiers (scale first to avoid conflict on
  // macOS)
  if (scaleMod) {
    const qreal step = (num_steps > 0) ? std::pow(kScaleStep, num_steps)
                                       : std::pow(1.0 / kScaleStep, -num_steps);
    QList<QGraphicsItem *> targets = scene()->selectedItems();
    if (targets.isEmpty()) {
      if (auto *hit = itemAt(event->position().toPoint())) {
        targets << hit;
      }
    }
    // Exclude substrate from scaling
    for (int i = targets.size() - 1; i >= 0; --i) {
      if (dynamic_cast<SubstrateItem *>(targets[i]) != nullptr) {
        targets.removeAt(i);
      }
    }
    if (targets.isEmpty()) {
      event->accept();
      return;
    }
    for (QGraphicsItem *it : targets) {
      it->setTransformOriginPoint(it->boundingRect().center());
      const qreal newScale = std::clamp(it->scale() * step, 0.1, 10.0);
      it->setScale(newScale);
    }
    event->accept();
    return;
  }

  if (rotateMod) {
    const qreal delta = (num_steps > 0 ? kRotateStepDeg : -kRotateStepDeg) *
                        std::abs(num_steps);
    QList<QGraphicsItem *> targets = scene()->selectedItems();
    if (targets.isEmpty()) {
      if (auto *hit = itemAt(event->position().toPoint())) {
        targets << hit;
      }
    }
    // Exclude substrate from rotation
    for (int i = targets.size() - 1; i >= 0; --i) {
      if (dynamic_cast<SubstrateItem *>(targets[i]) != nullptr) {
        targets.removeAt(i);
      }
    }
    if (targets.isEmpty()) {
      event->accept();
      return;
    }
    for (QGraphicsItem *it : targets) {
      it->setRotation(it->rotation() + delta);
    }
    event->accept();
    return;
  }

  qreal factor = (num_steps > 0) ? std::pow(kZoomStep, num_steps)
                                 : std::pow(1.0 / kZoomStep, -num_steps);
  applyZoom(factor);
}

void EditorView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::MiddleButton) {
    panning_ = true;
    last_mouse_pos_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
    return;
  }
  if (event->button() == Qt::LeftButton) {
    // Start panning only if click on empty space; otherwise let items handle
    // drag
    if (itemAt(event->pos()) == nullptr) {
      panning_ = true;
      last_mouse_pos_ = event->pos();
      setCursor(Qt::ClosedHandCursor);
      event->accept();
      return;
    }
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
                   event->button() == Qt::LeftButton)) {
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
