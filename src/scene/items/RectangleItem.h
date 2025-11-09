#pragma once

#include <QGraphicsRectItem>
#include "scene/ISceneObject.h"

class RectangleItem : public QGraphicsRectItem, public ISceneObject {
public:
    explicit RectangleItem(const QRectF &rect, QGraphicsItem *parent = nullptr);
    ~RectangleItem() override = default;

    // ISceneObject interface
    QWidget* create_properties_widget(QWidget *parent) override;
};

