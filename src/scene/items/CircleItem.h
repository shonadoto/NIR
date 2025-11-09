#pragma once

#include <QGraphicsEllipseItem>
#include "scene/ISceneObject.h"

class CircleItem : public QGraphicsEllipseItem, public ISceneObject {
public:
    explicit CircleItem(qreal radius, QGraphicsItem *parent = nullptr);
    ~CircleItem() override = default;

    QWidget* create_properties_widget(QWidget *parent) override;
};

