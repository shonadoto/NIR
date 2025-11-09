#include "PropertiesBar.h"

#include <QLabel>
#include <QLineEdit>
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

    type_label_ = new QLabel("Type: ", this);
    QFont f = type_label_->font();
    f.setBold(true);
    type_label_->setFont(f);
    layout_->addWidget(type_label_);

    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText("Object name");
    connect(name_edit_, &QLineEdit::textChanged, this, [this](const QString &text){
        if (updating_ || !current_item_) {
            return;
        }
        current_item_->set_name(text);
        emit name_changed(text);
    });
    layout_->addWidget(name_edit_);

    layout_->addStretch();

    setMinimumWidth(kMinPropertiesBarWidthPx);
    setFixedWidth(kDefaultPropertiesBarWidthPx);
    preferred_width_ = kDefaultPropertiesBarWidthPx;
}

PropertiesBar::~PropertiesBar() = default;

void PropertiesBar::set_selected_item(ISceneObject *item, const QString &name) {
    current_item_ = item;
    updating_ = true;

    // Remove old content
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
    if (!current_item_) {
        clear();
        updating_ = false;
        return;
    }
    // Update type and name
    QString type = current_item_->type_name();
    type[0] = type[0].toUpper(); // capitalize first letter
    type_label_->setText(QString("Type: %1").arg(type));
    name_edit_->setText(name);

    // Create new content from item
    content_widget_ = current_item_->create_properties_widget(this);
    if (content_widget_) {
        layout_->insertWidget(2, content_widget_);
    }

    updating_ = false;
}

void PropertiesBar::update_name(const QString &name) {
    if (!current_item_) {
        return;
    }
    updating_ = true;
    name_edit_->setText(name);
    updating_ = false;
}

void PropertiesBar::clear() {
    current_item_ = nullptr;
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
}
