#pragma once

#include <QGraphicsLineItem>
#include "scene/ISceneObject.h"

class StickItem : public QGraphicsLineItem, public ISceneObject {
public:
    explicit StickItem(const QLineF &line, QGraphicsItem *parent = nullptr);
    ~StickItem() override = default;

    QWidget* create_properties_widget(QWidget *parent) override;
};

