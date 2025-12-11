#include "EditorArea.h"

#include <QGraphicsScene>
#include <QTimer>
#include <QVBoxLayout>

#include "ui/EditorView.h"
#include "ui/editor/SubstrateItem.h"

namespace {
constexpr int kDefaultSubstrateWidth = 1000;
constexpr int kDefaultSubstrateHeight = 1000;
}  // namespace
EditorArea::EditorArea(QWidget* parent)
    : QWidget(parent), view_(new EditorView(this)) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  layout->addWidget(view_);

  init_scene();
}

EditorArea::~EditorArea() = default;

auto EditorArea::scene() const -> QGraphicsScene* {
  return scene_;
}

void EditorArea::init_scene() {
  scene_ = new QGraphicsScene(this);
  view_->setScene(scene_);

  substrate_ =
    new SubstrateItem(QSizeF(kDefaultSubstrateWidth, kDefaultSubstrateHeight));
  scene_->addItem(substrate_);
  scene_->setSceneRect(substrate_->boundingRect());

  fit_to_substrate();
}

void EditorArea::fit_to_substrate() {
  if (substrate_ == nullptr) {
    return;
  }
  view_->fitToItem(substrate_);
}

void EditorArea::set_substrate_size(const QSizeF& size) {
  if (substrate_ == nullptr) {
    return;
  }
  substrate_->set_size(size);
  scene_->setSceneRect(substrate_->boundingRect());
  fit_to_substrate();
}

auto EditorArea::substrate_size() const -> QSizeF {
  if (substrate_ == nullptr) {
    return {};
  }
  return substrate_->size();
}

auto EditorArea::substrate_center() const -> QPointF {
  if (substrate_ == nullptr) {
    return {0, 0};
  }
  return substrate_->sceneBoundingRect().center();
}

void EditorArea::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (!fitted_once_) {
    QTimer::singleShot(0, this, [this] {
      if (!fitted_once_) {
        fit_to_substrate();
        fitted_once_ = true;
      }
    });
  }
}
