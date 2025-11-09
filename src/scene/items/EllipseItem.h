#pragma once

#include <QGraphicsEllipseItem>
#include "scene/ISceneObject.h"

class EllipseItem : public QGraphicsEllipseItem, public ISceneObject {
public:
    explicit EllipseItem(const QRectF &rect, QGraphicsItem *parent = nullptr);
    ~EllipseItem() override = default;

    QWidget* create_properties_widget(QWidget *parent) override;
};

