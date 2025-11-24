#pragma once

#include <QGraphicsLineItem>
#include "scene/ISceneObject.h"

class StickItem : public QGraphicsLineItem, public ISceneObject {
public:
    explicit StickItem(const QLineF &line, QGraphicsItem *parent = nullptr);
    ~StickItem() override = default;

    QWidget* create_properties_widget(QWidget *parent) override;
    QJsonObject to_json() const override;
    void from_json(const QJsonObject &json) override;
    QString type_name() const override { return QStringLiteral("stick"); }
    QString name() const override { return name_; }
    void set_name(const QString &name) override;

private:
    QString name_ {"Stick"};
};

