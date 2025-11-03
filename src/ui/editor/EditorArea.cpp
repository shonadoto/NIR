#include "EditorArea.h"
#include "ui/EditorView.h"
#include "ui/editor/SubstrateItem.h"

#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QTimer>

namespace {
constexpr int kDefaultSubstrateWidth = 1000;
constexpr int kDefaultSubstrateHeight = 1000;
}
EditorArea::EditorArea(QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    view_ = new EditorView(this);
    layout->addWidget(view_);

    init_scene();
}

EditorArea::~EditorArea() = default;

void EditorArea::init_scene() {
    scene_ = std::make_unique<QGraphicsScene>(this);
    view_->setScene(scene_.get());

    substrate_ = new SubstrateItem(QSizeF(kDefaultSubstrateWidth, kDefaultSubstrateHeight));
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

void EditorArea::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!fitted_once_) {
        QTimer::singleShot(0, this, [this]{
            if (!fitted_once_) {
                fit_to_substrate();
                fitted_once_ = true;
            }
        });
    }
}


