#pragma once

#include <QGraphicsEllipseItem>
#include "scene/ISceneObject.h"

class CircleItem : public QGraphicsEllipseItem, public ISceneObject {
public:
    explicit CircleItem(qreal radius, QGraphicsItem *parent = nullptr);
    ~CircleItem() override = default;

    QWidget* create_properties_widget(QWidget *parent) override;
    QJsonObject to_json() const override;
    void from_json(const QJsonObject &json) override;
    QString type_name() const override { return QStringLiteral("circle"); }
    QString name() const override { return name_; }
    void set_name(const QString &name) override { name_ = name; }

private:
    QString name_ {"Circle"};
};

