#pragma once

#include <QString>

class EditorArea;
class ObjectTreeModel;

class ProjectSerializer {
public:
    static bool save_to_file(const QString &filename, EditorArea *editor_area, ObjectTreeModel *model);
    static bool load_from_file(const QString &filename, EditorArea *editor_area, ObjectTreeModel *model);

private:
    ProjectSerializer() = default;
};


