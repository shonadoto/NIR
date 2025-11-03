#include "ObjectsBar.h"

#include <QTreeView>
#include <QVBoxLayout>

namespace {
constexpr int kMinObjectsBarWidthPx = 220;
constexpr int kDefaultObjectsBarWidthPx = 280;
}

ObjectsBar::ObjectsBar(QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tree_view_ = new QTreeView(this);
    layout->addWidget(tree_view_);

    preferred_width_ = kDefaultObjectsBarWidthPx;
    last_visible_width_ = 0;
    setMinimumWidth(kMinObjectsBarWidthPx);
    setFixedWidth(preferred_width_);
}

ObjectsBar::~ObjectsBar() = default;

void ObjectsBar::setPreferredWidth(int w) {
    preferred_width_ = w;
    setFixedWidth(preferred_width_);
}

void ObjectsBar::hideEvent(QHideEvent *event) {
    last_visible_width_ = width();
    QWidget::hideEvent(event);
}

void ObjectsBar::showEvent(QShowEvent *event) {
    const int target = last_visible_width_ > 0 ? last_visible_width_ : preferred_width_;
    setFixedWidth(target);
    QWidget::showEvent(event);
}


