#pragma once

#include <QWidget>

class QFormLayout;
class QDoubleSpinBox;
class QPushButton;
class QGraphicsItem;

class PropertiesBar : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesBar(QWidget *parent = nullptr);
    ~PropertiesBar() override;

    void set_selected_item(QGraphicsItem *item);
    void clear();

signals:
    void property_changed();

private:
    void create_ui();
    void update_from_item();
    void apply_to_item();
    void on_value_changed();

private:
    QGraphicsItem *current_item_ {nullptr};
    QFormLayout *form_ {nullptr};
    QDoubleSpinBox *width_spin_ {nullptr};
    QDoubleSpinBox *height_spin_ {nullptr};
    QDoubleSpinBox *rotation_spin_ {nullptr};
    QPushButton *color_button_ {nullptr};
    QColor current_color_;
    bool updating_ {false};
    int preferred_width_ {280};
};
