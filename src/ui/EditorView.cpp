#include "EditorView.h"

#include <qnamespace.h>

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>
#include <QScrollBar>
#include <QWheelEvent>
#include <QWidget>
#include <QtGlobal>
#include <QtMath>
#include <algorithm>
#include <cmath>  // For std::pow, std::abs

#include "ui/editor/SubstrateItem.h"

namespace {
constexpr qreal kZoomStep = 1.15;    // per mouse wheel notch (symmetrical)
constexpr qreal kMinScale = 0.70;    // 70%
constexpr qreal kMaxScale = 100.0;   // 10000%
constexpr qreal kRotateStepDeg = 5;  // rotation step per notch when Ctrl held
constexpr qreal kScaleStep = 1.05;  // item scale step per notch when Shift held
}  // namespace

EditorView::EditorView(QWidget* parent) : QGraphicsView(parent) {
  // Scene will be set by EditorArea
  setRenderHint(QPainter::Antialiasing, true);
  setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  setDragMode(QGraphicsView::NoDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setAlignment(Qt::AlignCenter);
}

EditorView::~EditorView() = default;

void EditorView::fitToItem(const QGraphicsItem* item) {
  if (item == nullptr) {
    return;
  }
  const QRectF bounding_rect = item->sceneBoundingRect();
  if (bounding_rect.isEmpty()) {
    return;
  }
  fitInView(bounding_rect, Qt::KeepAspectRatio);
  centerOn(item);
  // Update scale_ to reflect current transform approximately
  scale_ = transform().m11();
}

// necessary for wheel event handling with multiple modifiers
void EditorView::wheelEvent(
  QWheelEvent* event) {  // NOLINT(readability-function-cognitive-complexity)
  const QPoint num_degrees = event->angleDelta() / 8;
  if (num_degrees.y() == 0) {
    QGraphicsView::wheelEvent(event);
    return;
  }

  const int num_steps = num_degrees.y() / 15;  // 15 deg per notch
  const auto mods = event->modifiers();

#ifdef Q_OS_MACOS
  const bool rotate_mod = (mods & Qt::ControlModifier) != 0;  // Cmd
  const bool scale_mod = (mods & Qt::AltModifier) != 0;       // Alt
#else
  const bool rotate_mod = (mods & Qt::ControlModifier) != 0;  // Ctrl
  const bool scale_mod = (mods & Qt::MetaModifier) != 0;      // Meta/Win key
#endif

  // Transform selected items with modifiers (scale first to avoid conflict on
  // macOS)
  if (scale_mod) {
    if (scene() == nullptr) {
      event->accept();
      return;
    }
    const qreal step = (num_steps > 0) ? std::pow(kScaleStep, num_steps)
                                       : std::pow(1.0 / kScaleStep, -num_steps);
    QList<QGraphicsItem*> targets = scene()->selectedItems();
    if (targets.isEmpty()) {
      if (auto* hit = itemAt(event->position().toPoint())) {
        targets << hit;
      }
    }
    // Exclude substrate from scaling
    for (int i = static_cast<int>(targets.size()) - 1; i >= 0; --i) {
      if (dynamic_cast<SubstrateItem*>(targets[i]) != nullptr) {
        targets.removeAt(i);
      }
    }
    if (targets.isEmpty()) {
      event->accept();
      return;
    }
    for (QGraphicsItem* item : targets) {
      item->setTransformOriginPoint(item->boundingRect().center());
      const qreal new_scale = std::clamp(item->scale() * step, 0.1, 10.0);
      item->setScale(new_scale);
    }
    event->accept();
    return;
  }

  if (rotate_mod) {
    if (scene() == nullptr) {
      event->accept();
      return;
    }
    const qreal delta =
      (num_steps > 0 ? kRotateStepDeg : -kRotateStepDeg) * std::abs(num_steps);
    QList<QGraphicsItem*> targets = scene()->selectedItems();
    if (targets.isEmpty()) {
      if (auto* hit = itemAt(event->position().toPoint())) {
        targets << hit;
      }
    }
    // Exclude substrate from rotation
    for (int i = static_cast<int>(targets.size()) - 1; i >= 0; --i) {
      if (dynamic_cast<SubstrateItem*>(targets[i]) != nullptr) {
        targets.removeAt(i);
      }
    }
    if (targets.isEmpty()) {
      event->accept();
      return;
    }
    for (QGraphicsItem* item : targets) {
      item->setRotation(item->rotation() + delta);
    }
    event->accept();
    return;
  }

  const qreal factor = (num_steps > 0) ? std::pow(kZoomStep, num_steps)
                                       : std::pow(1.0 / kZoomStep, -num_steps);
  applyZoom(factor);
}

void EditorView::mousePressEvent(QMouseEvent* event) {
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

void EditorView::mouseMoveEvent(QMouseEvent* event) {
  if (panning_) {
    const QPoint delta = event->pos() - last_mouse_pos_;
    last_mouse_pos_ = event->pos();
    if (auto* h_scroll = horizontalScrollBar()) {
      h_scroll->setValue(h_scroll->value() - delta.x());
    }
    if (auto* v_scroll = verticalScrollBar()) {
      v_scroll->setValue(v_scroll->value() - delta.y());
    }
    event->accept();
    return;
  }
  QGraphicsView::mouseMoveEvent(event);
}

void EditorView::mouseReleaseEvent(QMouseEvent* event) {
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
