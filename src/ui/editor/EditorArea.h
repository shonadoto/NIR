#pragma once

#include <QWidget>

class EditorView;
class SubstrateItem;
class QGraphicsScene;

class EditorArea : public QWidget {
    Q_OBJECT
public:
    explicit EditorArea(QWidget *parent = nullptr);
    ~EditorArea() override;

    EditorView* view() const { return view_; }
    void fit_to_substrate();

private:
    void init_scene();
    void showEvent(QShowEvent *event) override;

private:
    EditorView *view_ {nullptr};
    std::unique_ptr<QGraphicsScene> scene_;
    SubstrateItem *substrate_ {nullptr};
    bool fitted_once_ {false};
};


