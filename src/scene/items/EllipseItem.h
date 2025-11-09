#pragma once

#include <QGraphicsEllipseItem>
#include "scene/ISceneObject.h"

class EllipseItem : public QGraphicsEllipseItem, public ISceneObject {
public:
    explicit EllipseItem(const QRectF &rect, QGraphicsItem *parent = nullptr);
    ~EllipseItem() override = default;

    QWidget* create_properties_widget(QWidget *parent) override;
    QJsonObject to_json() const override;
    void from_json(const QJsonObject &json) override;
    QString type_name() const override { return QStringLiteral("ellipse"); }
    QString name() const override { return name_; }
    void set_name(const QString &name) override { name_ = name; }

private:
    QString name_ {"Ellipse"};
};

