#pragma once

#include <QWidget>

class EditorView;

class EditorArea : public QWidget {
    Q_OBJECT
public:
    explicit EditorArea(QWidget *parent = nullptr);
    ~EditorArea() override;

    EditorView* view() const { return view_; }

private:
    EditorView *view_ {nullptr};
};


