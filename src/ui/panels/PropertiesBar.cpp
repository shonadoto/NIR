#include "PropertiesBar.h"

#include <QLabel>
#include <QVBoxLayout>
#include "scene/ISceneObject.h"

namespace {
constexpr int kMinPropertiesBarWidthPx = 220;
constexpr int kDefaultPropertiesBarWidthPx = 280;
}

PropertiesBar::PropertiesBar(QWidget *parent)
    : QWidget(parent) {
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(8, 8, 8, 8);
    layout_->setSpacing(4);

    title_ = new QLabel("Properties", this);
    QFont f = title_->font();
    f.setBold(true);
    title_->setFont(f);
    layout_->addWidget(title_);

    layout_->addStretch();

    setMinimumWidth(kMinPropertiesBarWidthPx);
    setFixedWidth(kDefaultPropertiesBarWidthPx);
    preferred_width_ = kDefaultPropertiesBarWidthPx;
}

PropertiesBar::~PropertiesBar() = default;

void PropertiesBar::set_selected_item(ISceneObject *item) {
    current_item_ = item;
    // Remove old content
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
    if (!current_item_) {
        clear();
        return;
    }
    // Create new content from item
    content_widget_ = current_item_->create_properties_widget(this);
    if (content_widget_) {
        layout_->insertWidget(1, content_widget_);
    }
}

void PropertiesBar::clear() {
    current_item_ = nullptr;
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
}
