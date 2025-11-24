#pragma once

#include <QGraphicsRectItem>
#include "scene/ISceneObject.h"

class RectangleItem : public QGraphicsRectItem, public ISceneObject {
public:
    explicit RectangleItem(const QRectF &rect, QGraphicsItem *parent = nullptr);
    ~RectangleItem() override = default;

    // ISceneObject interface
    QWidget* create_properties_widget(QWidget *parent) override;
    QJsonObject to_json() const override;
    void from_json(const QJsonObject &json) override;
    QString type_name() const override { return QStringLiteral("rectangle"); }
    QString name() const override { return name_; }
    void set_name(const QString &name) override;

private:
    QString name_ {"Rectangle"};
};

