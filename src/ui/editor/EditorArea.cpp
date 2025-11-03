#include "EditorArea.h"
#include "ui/EditorView.h"

#include <QVBoxLayout>

EditorArea::EditorArea(QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    view_ = new EditorView(this);
    layout->addWidget(view_);
}

EditorArea::~EditorArea() = default;


