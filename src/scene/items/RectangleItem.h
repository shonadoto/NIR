#pragma once

#include <QGraphicsRectItem>
#include "scene/ISceneObject.h"
#include <functional>

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
    void set_geometry_changed_callback(std::function<void()> callback) override;
    void clear_geometry_changed_callback() override;
    void set_material_model(class MaterialModel *material) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QString name_ {"Rectangle"};
    std::function<void()> geometry_changed_callback_;
    class MaterialModel *material_model_ {nullptr};

    void notify_geometry_changed() const;
};

