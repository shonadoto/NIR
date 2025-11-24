#include "PropertiesBar.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include "scene/ISceneObject.h"
#include "model/MaterialPreset.h"
#include "model/ObjectTreeModel.h"
#include "utils/Logging.h"
#include <QGraphicsItem>
#include <QGraphicsScene>

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

    setup_type_selector();
    setup_material_selector(); // Initialize material controls early

    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText("Object name");
    // Use editingFinished instead of textChanged to allow temporary empty state during editing
    connect(name_edit_, &QLineEdit::editingFinished, this, [this]{
        if (updating_) {
            return;
        }
        QString trimmed = name_edit_->text().trimmed();

        if (current_preset_) {
            // Handle preset name change
            if (trimmed.isEmpty()) {
                // Restore previous name
                updating_ = true;
                name_edit_->setText(current_preset_->name());
                updating_ = false;
                return;
            }
            if (trimmed != current_preset_->name()) {
                emit preset_name_changed(current_preset_, trimmed);
            }
            return;
        }

        if (!current_item_) {
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
        if (trimmed.isEmpty()) {
            // Restore previous name
            updating_ = true;
            name_edit_->setText(current_item_->name());
            updating_ = false;
            return;
        }
        current_item_->set_name(trimmed);
        emit name_changed(trimmed);
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

    // Update type selector
    if (is_inclusion_item()) {
        type_combo_->setVisible(true);
        QString currentType = current_item_->type_name();
        int index = type_combo_->findData(currentType);
        if (index >= 0) {
            updating_ = true;
            type_combo_->setCurrentIndex(index);
            updating_ = false;
        }
        // Show material selector for inclusions (will be shown after content widget)
        update_material_ui();
    } else {
        type_combo_->setVisible(false);
        material_combo_->setVisible(false);
        material_color_btn_->setVisible(false);
    }

    // Create new content from item (object-specific properties: size, rotation, etc.)
    content_widget_ = current_item_->create_properties_widget(this);
    int insertIndex = 3; // After name_edit (type_label=0, type_combo=1, name_edit=2)
    if (content_widget_) {
        layout_->insertWidget(insertIndex, content_widget_);
        insertIndex++; // Material controls will go after content_widget
        // Disable color editing in content widget if material preset is selected
        if (item_material_ && content_widget_) {
            // Find and disable color button in content widget
            QList<QPushButton*> buttons = content_widget_->findChildren<QPushButton*>();
            for (auto *btn : buttons) {
                if (btn->text().contains("Color", Qt::CaseInsensitive)) {
                    btn->setEnabled(false);
                }
            }
        }
    }

    // Add material controls for inclusions (after object properties)
    if (is_inclusion_item()) {
        // Remove from layout if already added
        if (material_combo_->parent() == this) {
            layout_->removeWidget(material_combo_);
        }
        if (material_color_btn_->parent() == this) {
            layout_->removeWidget(material_color_btn_);
        }
        // Insert after content_widget
        layout_->insertWidget(insertIndex, material_combo_);
        layout_->insertWidget(insertIndex + 1, material_color_btn_);
        material_combo_->setVisible(true);
        material_color_btn_->setVisible(true);
    }

    updating_ = false;
}

void PropertiesBar::update_name(const QString &name) {
    if (current_preset_) {
        updating_ = true;
        name_edit_->setText(name);
        updating_ = false;
        return;
    }
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
    current_preset_ = nullptr;
    item_material_ = nullptr;
    updating_ = true;
    name_edit_->clear();
    type_label_->setText("Type: ");
    type_combo_->setVisible(false);
    material_combo_->setVisible(false);
    material_color_btn_->setVisible(false);
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }
    updating_ = false;
}

void PropertiesBar::setup_type_selector() {
    type_combo_ = new QComboBox(this);
    type_combo_->addItem("Circle", "circle");
    type_combo_->addItem("Rectangle", "rectangle");
    type_combo_->addItem("Ellipse", "ellipse");
    type_combo_->addItem("Stick", "stick");
    type_combo_->setVisible(false);

    connect(type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (updating_ || !current_item_ || !is_inclusion_item()) {
            return;
        }
        QString newType = type_combo_->itemData(index).toString();
        QString currentType = current_item_->type_name();
        if (newType != currentType) {
            emit type_changed(current_item_, newType);
        }
    });

    layout_->addWidget(type_combo_);
}

void PropertiesBar::setup_material_selector() {
    material_combo_ = new QComboBox(this);
    material_combo_->addItem("Custom", QVariant::fromValue<MaterialPreset*>(nullptr));
    material_combo_->setVisible(false);

    connect(material_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (updating_ || !current_item_ || !model_) {
            return;
        }
        MaterialPreset *preset = material_combo_->itemData(index).value<MaterialPreset*>();
        item_material_ = preset;

        // Enable/disable color button in content widget
        if (content_widget_) {
            QList<QPushButton*> buttons = content_widget_->findChildren<QPushButton*>();
            for (auto *btn : buttons) {
                if (btn->text().contains("Color", Qt::CaseInsensitive)) {
                    btn->setEnabled(preset == nullptr); // Enable only for Custom
                }
            }
        }

        // Update material UI (updates color button display)
        updating_ = true;
        update_material_color_button();
        updating_ = false;

        // Emit signal to apply color to item
        emit item_material_changed(current_item_, preset);
    });

    material_color_btn_ = new QPushButton("Material Color", this);
    material_color_btn_->setVisible(false);
    connect(material_color_btn_, &QPushButton::clicked, this, [this] {
        if (updating_) return;
        QColor currentColor;
        if (current_preset_) {
            currentColor = current_preset_->fill_color();
        } else if (current_item_ && item_material_) {
            currentColor = item_material_->fill_color();
        } else if (current_item_) {
            // Get color from item
            if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(current_item_)) {
                if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
                    currentColor = rectItem->brush().color();
                } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
                    currentColor = ellipseItem->brush().color();
                } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
                    currentColor = lineItem->pen().color();
                }
            }
        }

        QColor newColor = QColorDialog::getColor(currentColor, material_color_btn_, "Choose Material Color", QColorDialog::ShowAlphaChannel);
        if (newColor.isValid()) {
            if (current_preset_) {
                current_preset_->set_fill_color(newColor);
                emit preset_color_changed(current_preset_, newColor);
            } else if (current_item_ && item_material_) {
                item_material_->set_fill_color(newColor);
                emit item_material_changed(current_item_, item_material_);
            } else if (current_item_) {
                // Apply to item directly (Custom material)
                if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(current_item_)) {
                    if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
                        rectItem->setBrush(QBrush(newColor));
                    } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
                        ellipseItem->setBrush(QBrush(newColor));
                    } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
                        QPen pen = lineItem->pen();
                        pen.setColor(newColor);
                        lineItem->setPen(pen);
                    }
                }
            }
            update_material_color_button();
        }
    });

    // Material controls will be added dynamically after content_widget
    // Don't add them here to avoid wrong order
}

void PropertiesBar::set_model(ObjectTreeModel *model) {
    model_ = model;
    update_material_ui();
}

void PropertiesBar::set_selected_preset(MaterialPreset *preset) {
    current_preset_ = preset;
    current_item_ = nullptr;
    item_material_ = nullptr;
    updating_ = true;

    // Remove old content
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }

    if (!current_preset_) {
        clear();
        updating_ = false;
        return;
    }

    // Update UI for preset
    type_label_->setText("Type: Material Preset");
    name_edit_->setText(current_preset_->name());
    type_combo_->setVisible(false);
    material_combo_->setVisible(false);

    // Remove material_color_btn from layout if needed and re-add after name_edit
    if (material_color_btn_->parent() == this) {
        layout_->removeWidget(material_color_btn_);
    }
    // Insert after name_edit (index 2: type_label=0, name_edit=2, no type_combo for preset)
    layout_->insertWidget(2, material_color_btn_);
    material_color_btn_->setVisible(true);
    material_color_btn_->setEnabled(true);

    update_material_color_button();

    updating_ = false;
}

void PropertiesBar::update_material_ui() {
    if (!model_ || !current_item_) {
        return;
    }

    updating_ = true;

    // Update material combo with available presets
    material_combo_->clear();
    material_combo_->addItem("Custom", QVariant::fromValue<MaterialPreset*>(nullptr));
    for (auto *preset : model_->get_presets()) {
        material_combo_->addItem(preset->name(), QVariant::fromValue<MaterialPreset*>(preset));
    }

    // Set current selection
    if (item_material_) {
        for (int i = 0; i < material_combo_->count(); ++i) {
            if (material_combo_->itemData(i).value<MaterialPreset*>() == item_material_) {
                material_combo_->setCurrentIndex(i);
                break;
            }
        }
    } else {
        material_combo_->setCurrentIndex(0); // Custom
    }

    // Update color button visibility and state
    material_color_btn_->setEnabled(true);
    update_material_color_button();

    updating_ = false;
}

void PropertiesBar::update_material_color_button() {
    QColor color;
    if (current_preset_) {
        color = current_preset_->fill_color();
    } else if (item_material_) {
        color = item_material_->fill_color();
    } else if (current_item_) {
        // Get color from item
        if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(current_item_)) {
            if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
                color = rectItem->brush().color();
            } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
                color = ellipseItem->brush().color();
            } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
                color = lineItem->pen().color();
            }
        }
    }
    QString style = QString("background-color: %1;").arg(color.name());
    material_color_btn_->setStyleSheet(style);
}

bool PropertiesBar::is_inclusion_item() const {
    if (!current_item_) {
        return false;
    }
    QString type = current_item_->type_name();
    return type != "substrate";
}
