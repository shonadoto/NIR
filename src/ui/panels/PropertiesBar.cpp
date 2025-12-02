#include "PropertiesBar.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include "scene/ISceneObject.h"
#include "model/MaterialModel.h"
#include "model/ObjectTreeModel.h"
#include "utils/Logging.h"
#include "ui/bindings/ShapeModelBinder.h"
#include "ui/utils/ColorUtils.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMetaType>

Q_DECLARE_METATYPE(MaterialModel*)

namespace {
constexpr int kMinPropertiesBarWidthPx = 220;
constexpr int kDefaultPropertiesBarWidthPx = 280;
}

PropertiesBar::PropertiesBar(QWidget *parent)
    : QWidget(parent) {
    qRegisterMetaType<MaterialModel*>("MaterialModel*");
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(8, 8, 8, 8);
    layout_->setSpacing(4);

    type_label_ = new QLabel("", this);
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

        if (current_material_) {
            // Handle material name change
            if (trimmed.isEmpty()) {
                // Restore previous name
                updating_ = true;
                name_edit_->setText(QString::fromStdString(current_material_->name()));
                updating_ = false;
                return;
            }
            if (trimmed != QString::fromStdString(current_material_->name())) {
                emit material_name_changed(current_material_, trimmed);
            }
            return;
        }

        if (!current_item_ && !current_model_) {
            return;
        }
        if (trimmed.isEmpty()) {
            // Restore previous name
            updating_ = true;
            if (current_model_) {
                name_edit_->setText(QString::fromStdString(current_model_->name()));
            } else if (current_item_) {
                name_edit_->setText(current_item_->name());
            }
            updating_ = false;
            return;
        }
        if (current_model_) {
            current_model_->set_name(trimmed.toStdString());
        } else if (current_item_) {
        // Validate item is still valid before accessing
        if (auto *graphicsItem = dynamic_cast<QGraphicsItem*>(current_item_)) {
            if (!graphicsItem->scene()) {
                LOG_WARN() << "PropertiesBar: name_changed callback - item no longer in scene";
                current_item_ = nullptr;
                return;
            }
        }
        current_item_->set_name(trimmed);
        }
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

    disconnect_model_signals();
    current_item_ = item;
    current_material_ = nullptr;
    current_material_shared_.reset();
    if (shape_binder_ && current_item_ && current_item_->type_name() != "substrate") {
        current_model_ = shape_binder_->bind_shape(current_item_);
        if (current_model_) {
            if (auto material = current_model_->material()) {
                item_material_ = material.get();
            } else {
                item_material_ = nullptr;
            }
        }
    } else {
        current_model_.reset();
        item_material_ = nullptr;
    }
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
    if (type == "Substrate") {
        type_label_->setText("Substrate");
    } else {
        type_label_->setText(QString("Inclusion"));
    }
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
        material_color_btn_->setEnabled(can_edit_material_color());
    }

    updating_ = false;
    connect_model_signals();
}

void PropertiesBar::update_name(const QString &name) {
    if (current_material_) {
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

void PropertiesBar::disconnect_model_signals() {
    if (current_material_shared_ && material_connection_id_ != 0) {
        current_material_shared_->on_changed().disconnect(material_connection_id_);
        material_connection_id_ = 0;
    }
    if (current_model_ && shape_model_connection_id_ != 0) {
        current_model_->on_changed().disconnect(shape_model_connection_id_);
        shape_model_connection_id_ = 0;
    }
}

void PropertiesBar::connect_model_signals() {
    // Connect to material signals if material is selected
    if (current_material_shared_) {
        LOG_DEBUG() << "PropertiesBar: Connecting to material signals for material=" << current_material_shared_.get();
        material_connection_id_ = current_material_shared_->on_changed().connect([this](const ModelChange &change) {
            LOG_DEBUG() << "PropertiesBar: Material change signal received, type=" << static_cast<int>(change.type);
            if (change.type == ModelChange::Type::NameChanged && current_material_shared_) {
                QString name = QString::fromStdString(current_material_shared_->name());
                LOG_DEBUG() << "PropertiesBar: Material name changed to=" << name.toStdString();
                if (!updating_) {
                    update_name(name);
                }
            }
        });
    }

    // Connect to shape model signals if shape is selected
    if (current_model_) {
        LOG_DEBUG() << "PropertiesBar: Connecting to shape model signals";
        shape_model_connection_id_ = current_model_->on_changed().connect([this](const ModelChange &change) {
            if (change.type == ModelChange::Type::NameChanged && current_model_) {
                QString name = QString::fromStdString(current_model_->name());
                if (!updating_) {
                    update_name(name);
                }
            }
        });
    }
}

void PropertiesBar::clear() {
    disconnect_model_signals();
    current_item_ = nullptr;
    current_material_ = nullptr;
    current_material_shared_.reset();
    current_model_.reset();
    item_material_ = nullptr;
    updating_ = true;
    name_edit_->clear();
    type_label_->setText("");
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
    material_combo_->addItem("Custom", QVariant::fromValue<MaterialModel*>(nullptr));
    material_combo_->setVisible(false);

    connect(material_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (updating_ || !current_item_ || !model_) {
            return;
        }
        MaterialModel *material = material_combo_->itemData(index).value<MaterialModel*>();
        item_material_ = material;

        // Apply to current shape model
        if (current_model_) {
            if (material) {
                auto shared = find_material(material);
                if (shared) {
                    current_model_->assign_material(shared);
                }
            } else {
                current_model_->clear_material();
            }
        }

        updating_ = true;
        update_material_color_button();
        updating_ = false;

        emit item_material_changed(current_item_, material);
        material_color_btn_->setEnabled(can_edit_material_color());
    });

    material_color_btn_ = new QPushButton("Material Color", this);
    material_color_btn_->setVisible(false);
    connect(material_color_btn_, &QPushButton::clicked, this, [this] {
        if (updating_) return;
        QColor currentColor;
        if (current_material_) {
            currentColor = to_qcolor(current_material_->color());
        } else if (current_item_ && item_material_) {
            currentColor = to_qcolor(item_material_->color());
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
            if (current_material_) {
                current_material_->set_color(to_model_color(newColor));
                emit material_color_changed(current_material_, newColor);
            } else if (current_item_ && item_material_) {
                item_material_->set_color(to_model_color(newColor));
                emit material_color_changed(item_material_, newColor);
            } else if (current_model_) {
                current_model_->set_custom_color(to_model_color(newColor));
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

void PropertiesBar::set_selected_material(MaterialModel *material) {
    disconnect_model_signals();
    current_material_ = material;
    current_item_ = nullptr;
    item_material_ = nullptr;
    current_model_.reset();

    // Find and store shared_ptr to material
    current_material_shared_.reset();
    if (material && model_) {
        current_material_shared_ = find_material(material);
        if (!current_material_shared_) {
            LOG_WARN() << "PropertiesBar: Could not find shared_ptr for material=" << material;
        } else {
            LOG_DEBUG() << "PropertiesBar: Found shared_ptr for material=" << material;
        }
    }

    updating_ = true;

    // Remove old content
    if (content_widget_) {
        layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }

    if (!current_material_) {
        clear();
        updating_ = false;
        return;
    }

    // Update UI for preset
    type_label_->setText("Material");
    name_edit_->setText(QString::fromStdString(current_material_->name()));
    type_combo_->setVisible(false);
    material_combo_->setVisible(false);

    // Remove material_color_btn from layout if needed and re-add after name_edit
    if (material_color_btn_->parent() == this) {
        layout_->removeWidget(material_color_btn_);
    }
    // Insert after name_edit (index 2: type_label=0, name_edit=2, no type_combo for preset)
    layout_->insertWidget(3, material_color_btn_);
    material_color_btn_->setVisible(true);
    material_color_btn_->setEnabled(true);

    update_material_color_button();

    updating_ = false;
    connect_model_signals();
}

void PropertiesBar::update_material_ui() {
    if (!model_ || !current_item_) {
        return;
    }

    updating_ = true;

    // Update material combo with available materials
    material_combo_->clear();
    material_combo_->addItem("Custom", QVariant::fromValue<MaterialModel*>(nullptr));
    if (auto *doc = model_->document()) {
        for (const auto &mat : doc->materials()) {
            material_combo_->addItem(QString::fromStdString(mat->name()),
                                     QVariant::fromValue<MaterialModel*>(mat.get()));
        }
    }

    // Set current selection
    if (item_material_) {
        for (int i = 0; i < material_combo_->count(); ++i) {
            if (material_combo_->itemData(i).value<MaterialModel*>() == item_material_) {
                material_combo_->setCurrentIndex(i);
                break;
            }
        }
    } else {
        material_combo_->setCurrentIndex(0); // Custom
    }

    // Update color button visibility and state
    update_material_color_button();

    updating_ = false;
}

void PropertiesBar::update_material_color_button() {
    QColor color;
    if (current_material_) {
        color = to_qcolor(current_material_->color());
    } else if (item_material_) {
        color = to_qcolor(item_material_->color());
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
    material_color_btn_->setEnabled(can_edit_material_color());
}

bool PropertiesBar::is_inclusion_item() const {
    if (!current_item_) {
        return false;
    }
    QString type = current_item_->type_name();
    return type != "substrate";
}

std::shared_ptr<MaterialModel> PropertiesBar::find_material(MaterialModel *raw) const {
    if (!raw || !model_) {
        return nullptr;
    }
    auto *doc = model_->document();
    if (!doc) {
        return nullptr;
    }
    for (const auto &material : doc->materials()) {
        if (material.get() == raw) {
            return material;
        }
    }
    return nullptr;
}

bool PropertiesBar::can_edit_material_color() const {
    if (current_material_) {
        return true;
    }
    if (!current_item_) {
        return false;
    }
    return item_material_ == nullptr;
}

