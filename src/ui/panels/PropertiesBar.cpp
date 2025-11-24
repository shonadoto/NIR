#include "PropertiesBar.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include "scene/ISceneObject.h"
#include "utils/Logging.h"
#include <QGraphicsItem>

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
        // Validate item is still valid before accessing
        if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(current_item_)) {
            if (!graphicsItem->scene()) {
                LOG_WARN() << "PropertiesBar: name_changed callback - item no longer in scene";
                current_item_ = nullptr;
                return;
            }
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

PropertiesBar::~PropertiesBar() {
    // Clear current item reference before destroying widgets
    current_item_ = nullptr;

    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
}

void PropertiesBar::set_selected_item(ISceneObject *item, const QString &name) {
    LOG_DEBUG() << "PropertiesBar::set_selected_item called with item=" << item << " name=" << name.toStdString();

    current_item_ = item;
    updating_ = true;

    // Remove old content
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
    if (!current_item_) {
        LOG_DEBUG() << "PropertiesBar::set_selected_item: item is null, clearing";
        clear();
        updating_ = false;
        return;
    }

    // Validate that item is still valid (check if it's a QGraphicsItem that's still in scene)
    if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(current_item_)) {
        if (!graphicsItem->scene()) {
            LOG_WARN() << "PropertiesBar::set_selected_item: item is not in scene, clearing. Item ptr=" << current_item_;
            clear();
            updating_ = false;
            return;
        }
    }

    // Update type and name - add try-catch for safety
    QString type;
    try {
        type = current_item_->type_name();
    } catch (...) {
        LOG_ERROR() << "PropertiesBar::set_selected_item: exception calling type_name() on item=" << current_item_;
        clear();
        updating_ = false;
        return;
    }
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
    // Validate item is still valid
    if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(current_item_)) {
        if (!graphicsItem->scene()) {
            LOG_WARN() << "PropertiesBar::update_name: item no longer in scene";
            current_item_ = nullptr;
            return;
        }
    }
    updating_ = true;
    name_edit_->setText(name);
    updating_ = false;
}

void PropertiesBar::clear() {
    current_item_ = nullptr;
    updating_ = true;
    name_edit_->clear();
    type_label_->setText("Type: ");
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
    updating_ = false;
}
