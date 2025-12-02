#include "ui/bindings/ShapeModelBinder.h"

#include "scene/ISceneObject.h"
#include "ui/utils/ColorUtils.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/CircleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/StickItem.h"
#include <QGraphicsItem>

#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QBrush>
#include <QPen>
#include <QLineF>
#include <QString>

namespace {
Color color_from_item(const ISceneObject *item) {
    if (auto graphicsItem = dynamic_cast<const QGraphicsItem*>(item)) {
        if (auto rect = dynamic_cast<const QGraphicsRectItem*>(graphicsItem)) {
            return to_model_color(rect->brush().color());
        }
        if (auto ellipse = dynamic_cast<const QGraphicsEllipseItem*>(graphicsItem)) {
            return to_model_color(ellipse->brush().color());
        }
        if (auto line = dynamic_cast<const QGraphicsLineItem*>(graphicsItem)) {
            return to_model_color(line->pen().color());
        }
    }
    return Color{};
}

void apply_color_to_item(ISceneObject *item, const Color &color) {
    QColor qcolor = to_qcolor(color);
    if (auto graphicsItem = dynamic_cast<QGraphicsItem*>(item)) {
        if (auto rect = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
            QBrush brush = rect->brush();
            brush.setColor(qcolor);
            rect->setBrush(brush);
        } else if (auto ellipse = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
            QBrush brush = ellipse->brush();
            brush.setColor(qcolor);
            ellipse->setBrush(brush);
        } else if (auto line = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
            QPen pen = line->pen();
            pen.setColor(qcolor);
            line->setPen(pen);
        }
    }
}

Point2D to_model_point(const QPointF &point) {
    return Point2D{point.x(), point.y()};
}

QPointF to_qpoint(const Point2D &point) {
    return QPointF(point.x, point.y);
}

}

ShapeModelBinder::ShapeModelBinder(DocumentModel &document)
    : document_(document) {}

namespace {
ShapeModel::ShapeType type_from_item(ISceneObject *item) {
    if (dynamic_cast<RectangleItem*>(item)) {
        return ShapeModel::ShapeType::Rectangle;
    }
    if (dynamic_cast<EllipseItem*>(item)) {
        return ShapeModel::ShapeType::Ellipse;
    }
    if (dynamic_cast<CircleItem*>(item)) {
        return ShapeModel::ShapeType::Circle;
    }
    if (dynamic_cast<StickItem*>(item)) {
        return ShapeModel::ShapeType::Stick;
    }
    return ShapeModel::ShapeType::Rectangle;
}
}

std::shared_ptr<ShapeModel> ShapeModelBinder::bind_shape(ISceneObject *item) {
    if (!item) {
        return nullptr;
    }
    auto it = bindings_.find(item);
    if (it != bindings_.end()) {
        return it->second.model;
    }

    auto model = document_.create_shape(type_from_item(item), item->name().toStdString());
    if (model->material()) {
        model->material()->set_color(color_from_item(item));
    }

    int connection_id = model->on_changed().connect([this, item](const ModelChange &change){
        handle_change(item, change);
    });

    bindings_[item] = Binding{model, connection_id};
    update_material_binding(item, bindings_[item]);
    update_model_geometry(item, model);
    item->set_geometry_changed_callback([this, item]{
        on_item_geometry_changed(item);
    });
    apply_color_to_item(item, model->material() ? model->material()->color() : Color{});
    return model;
}

std::shared_ptr<ShapeModel> ShapeModelBinder::attach_shape(ISceneObject *item, const std::shared_ptr<ShapeModel> &model) {
    if (!item || !model) {
        return nullptr;
    }
    if (bindings_.count(item)) {
        return bindings_[item].model;
    }

    int connection_id = model->on_changed().connect([this, item](const ModelChange &change){
        handle_change(item, change);
    });

    bindings_[item] = Binding{model, connection_id};
    update_material_binding(item, bindings_[item]);
    item->set_geometry_changed_callback([this, item]{
        on_item_geometry_changed(item);
    });
    apply_geometry(item, model);
    apply_color_to_item(item, model->material() ? model->material()->color() : Color{});
    return model;
}

std::shared_ptr<ShapeModel> ShapeModelBinder::model_for(ISceneObject *item) const {
    auto it = bindings_.find(item);
    if (it != bindings_.end()) {
        return it->second.model;
    }
    return nullptr;
}

QGraphicsItem* ShapeModelBinder::item_for(const std::shared_ptr<ShapeModel> &model) const {
    for (const auto &entry : bindings_) {
        if (entry.second.model == model) {
            return dynamic_cast<QGraphicsItem*>(entry.first);
        }
    }
    return nullptr;
}

void ShapeModelBinder::unbind_shape(ISceneObject *item) {
    auto it = bindings_.find(item);
    if (it == bindings_.end()) {
        return;
    }
    item->clear_geometry_changed_callback();
    detach_material_binding(it->second);
    if (auto model = it->second.model) {
        model->on_changed().disconnect(it->second.connection_id);
    }
    bindings_.erase(it);
}

void ShapeModelBinder::clear_bindings() {
    for (auto &entry : bindings_) {
        entry.first->clear_geometry_changed_callback();
        if (entry.second.model) {
            entry.second.model->on_changed().disconnect(entry.second.connection_id);
        }
        detach_material_binding(entry.second);
    }
    bindings_.clear();
}

void ShapeModelBinder::handle_change(ISceneObject *item, const ModelChange &change) {
    auto bindingIt = bindings_.find(item);
    if (bindingIt == bindings_.end()) {
        return;
    }
    auto model = bindingIt->second.model;
    if (!model) {
        return;
    }

    switch (change.type) {
        case ModelChange::Type::NameChanged:
            apply_name(item, model->name());
            break;
        case ModelChange::Type::MaterialChanged:
            update_material_binding(item, bindingIt->second);
            [[fallthrough]];
        case ModelChange::Type::ColorChanged: {
            Color color = model->material() ? model->material()->color() : Color{};
            apply_color_to_item(item, color);
            break;
        }
        case ModelChange::Type::GeometryChanged:
            apply_geometry(item, model);
            break;
        default:
            break;
    }
}

void ShapeModelBinder::apply_name(ISceneObject *item, const std::string &name) {
    item->set_name(QString::fromStdString(name));
}

void ShapeModelBinder::apply_color(ISceneObject *item, const Color &color) {
    apply_color_to_item(item, color);
}

Color ShapeModelBinder::extract_color(const ISceneObject *item) const {
    return color_from_item(item);
}

void ShapeModelBinder::update_material_binding(ISceneObject *item, Binding &binding) {
    detach_material_binding(binding);
    if (!binding.model) {
        return;
    }
    auto material = binding.model->material();
    if (!material) {
        return;
    }
    binding.bound_material = material;
    MaterialModel *material_ptr = material.get();
    binding.material_connection_id = material->on_changed().connect([this, item, material_ptr](const ModelChange &change){
        if (change.type != ModelChange::Type::ColorChanged) {
            return;
        }
        auto it = bindings_.find(item);
        if (it == bindings_.end()) {
            return;
        }
        if (!it->second.bound_material || it->second.bound_material.get() != material_ptr) {
            return;
        }
        apply_color(item, it->second.bound_material->color());
    });
}

void ShapeModelBinder::detach_material_binding(Binding &binding) {
    if (binding.bound_material && binding.material_connection_id != 0) {
        binding.bound_material->on_changed().disconnect(binding.material_connection_id);
    }
    binding.material_connection_id = 0;
    binding.bound_material.reset();
}

void ShapeModelBinder::update_model_geometry(ISceneObject *item, const std::shared_ptr<ShapeModel> &model) {
    if (!item || !model) {
        return;
    }
    auto graphicsItem = dynamic_cast<QGraphicsItem*>(item);
    if (!graphicsItem) {
        return;
    }
    auto bindingIt = bindings_.find(item);
    if (bindingIt != bindings_.end()) {
        bindingIt->second.suppress_model_geometry_signal = true;
    }
    model->set_position(to_model_point(graphicsItem->pos()));
    model->set_rotation_deg(graphicsItem->rotation());

    if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
        QRectF rect = rectItem->rect();
        model->set_size(Size2D{rect.width(), rect.height()});
    } else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
        QRectF rect = ellipseItem->rect();
        model->set_size(Size2D{rect.width(), rect.height()});
    } else if (auto lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
        const qreal length = lineItem->line().length();
        model->set_size(Size2D{length, lineItem->pen().widthF()});
    } else {
        QSizeF size = graphicsItem->boundingRect().size();
        model->set_size(Size2D{size.width(), size.height()});
    }

    if (bindingIt != bindings_.end()) {
        bindingIt->second.suppress_model_geometry_signal = false;
    }
}

void ShapeModelBinder::apply_geometry(ISceneObject *item, const std::shared_ptr<ShapeModel> &model) {
    if (!item || !model) {
        return;
    }
    auto graphicsItem = dynamic_cast<QGraphicsItem*>(item);
    if (!graphicsItem) {
        return;
    }
    auto it = bindings_.find(item);
    if (it != bindings_.end()) {
        if (it->second.suppress_model_geometry_signal) {
            return;
        }
        it->second.suppress_geometry_callback = true;
    }

    graphicsItem->setPos(to_qpoint(model->position()));
    graphicsItem->setRotation(model->rotation_deg());

    const Size2D size = model->size();
    if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(graphicsItem)) {
        QRectF rect = rectItem->rect();
        rect.setWidth(size.width);
        rect.setHeight(size.height);
        rectItem->setRect(rect);
        rectItem->setTransformOriginPoint(rectItem->boundingRect().center());
    } else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
        QRectF rect = ellipseItem->rect();
        rect.setWidth(size.width);
        rect.setHeight(size.height);
        ellipseItem->setRect(rect);
        ellipseItem->setTransformOriginPoint(ellipseItem->boundingRect().center());
    } else if (auto lineItem = dynamic_cast<QGraphicsLineItem*>(graphicsItem)) {
        const qreal halfLength = size.width / 2.0;
        lineItem->setLine(QLineF(-halfLength, 0.0, halfLength, 0.0));
        lineItem->setTransformOriginPoint(lineItem->boundingRect().center());
    }

    if (it != bindings_.end()) {
        it->second.suppress_geometry_callback = false;
    }
}

void ShapeModelBinder::on_item_geometry_changed(ISceneObject *item) {
    auto it = bindings_.find(item);
    if (it == bindings_.end()) {
        return;
    }
    if (it->second.suppress_geometry_callback) {
        return;
    }
    update_model_geometry(item, it->second.model);
}

