#pragma once

#include <QGraphicsItem>
#include "scene/ISceneObject.h"

class SubstrateItem : public QGraphicsItem, public ISceneObject {
public:
    explicit SubstrateItem(const QSizeF &size);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QSizeF size() const { return size_; }
    void set_size(const QSizeF &size);

    QColor fill_color() const { return fill_color_; }
    void set_fill_color(const QColor &color);

    // ISceneObject interface
    QWidget* create_properties_widget(QWidget *parent) override;
    QJsonObject to_json() const override;
    void from_json(const QJsonObject &json) override;
    QString type_name() const override { return QStringLiteral("substrate"); }

private:
    QSizeF size_;
    QColor fill_color_ {240, 240, 240};
};


