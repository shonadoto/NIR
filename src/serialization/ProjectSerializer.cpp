#include "ProjectSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGraphicsScene>

#include "model/ObjectTreeModel.h"
#include "scene/ISceneObject.h"
#include "scene/items/CircleItem.h"
#include "scene/items/EllipseItem.h"
#include "scene/items/RectangleItem.h"
#include "scene/items/StickItem.h"
#include "ui/editor/EditorArea.h"
#include "ui/editor/SubstrateItem.h"

bool ProjectSerializer::save_to_file(const QString &filename, EditorArea *editor_area, ObjectTreeModel *model) {
    if (!editor_area || !model) {
        return false;
    }

    QJsonObject root;
    root["version"] = "1.0";

    // Serialize substrate
    if (auto *substrate = editor_area->substrate_item()) {
        root["substrate"] = substrate->to_json();
    }

    // Serialize objects
    QJsonArray objects;
    auto *scene = editor_area->scene();
    if (scene) {
        for (QGraphicsItem *item : scene->items()) {
            if (auto *sceneObj = dynamic_cast<ISceneObject*>(item)) {
                if (sceneObj->type_name() != "substrate") {
                    QJsonObject itemObj = sceneObj->to_json();
                    // Store name from model
                    // (simplified: we don't track names in serializer yet, future enhancement)
                    objects.append(itemObj);
                }
            }
        }
    }
    root["objects"] = objects;

    QJsonDocument doc(root);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ProjectSerializer::load_from_file(const QString &filename, EditorArea *editor_area, ObjectTreeModel *model) {
    if (!editor_area || !model) {
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    // Check version (future: handle migrations)
    if (!root.contains("version")) {
        return false;
    }

    // Clear existing scene (except substrate) and model
    auto *scene = editor_area->scene();
    if (!scene) {
        return false;
    }

    // Clear model items first (will trigger tree updates)
    model->clear_items();

    QList<QGraphicsItem*> toRemove;
    for (QGraphicsItem *item : scene->items()) {
        if (dynamic_cast<SubstrateItem*>(item) == nullptr) {
            toRemove.append(item);
        }
    }
    for (auto *item : toRemove) {
        scene->removeItem(item);
        delete item;
    }

    // Restore substrate
    if (root.contains("substrate")) {
        if (auto *substrate = editor_area->substrate_item()) {
            substrate->from_json(root["substrate"].toObject());
            model->set_substrate(substrate);
        }
    }

    // Restore objects
    if (root.contains("objects")) {
        QJsonArray objects = root["objects"].toArray();
        for (const QJsonValue &val : objects) {
            QJsonObject obj = val.toObject();
            QString type = obj["type"].toString();
            ISceneObject *newItem = nullptr;

            if (type == "rectangle") {
                auto *rect = new RectangleItem(QRectF(0, 0, 100, 60));
                rect->from_json(obj);
                scene->addItem(rect);
                newItem = rect;
                model->add_item(rect, rect->name());
            } else if (type == "ellipse") {
                auto *ellipse = new EllipseItem(QRectF(0, 0, 100, 60));
                ellipse->from_json(obj);
                scene->addItem(ellipse);
                newItem = ellipse;
                model->add_item(ellipse, ellipse->name());
            } else if (type == "circle") {
                auto *circle = new CircleItem(40);
                circle->from_json(obj);
                scene->addItem(circle);
                newItem = circle;
                model->add_item(circle, circle->name());
            } else if (type == "stick") {
                auto *stick = new StickItem(QLineF(0, 0, 100, 0));
                stick->from_json(obj);
                scene->addItem(stick);
                newItem = stick;
                model->add_item(stick, stick->name());
            }
        }
    }

    return true;
}


