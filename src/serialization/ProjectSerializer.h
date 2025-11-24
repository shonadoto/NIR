#pragma once

#include <QString>

class DocumentModel;

class ProjectSerializer {
public:
    static bool save_to_file(const QString &filename, DocumentModel *document);
    static bool load_from_file(const QString &filename, DocumentModel *document);

private:
    ProjectSerializer() = default;
};


