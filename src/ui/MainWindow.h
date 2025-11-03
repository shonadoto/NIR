#pragma once

#include <QMainWindow>

class QTreeView;
class QToolBar;
class QWidget;
class QStackedWidget;
class QToolButton;
class PanelArea;
class EditorView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createActionsAndToolbar();
    void createActivityObjectsBarAndEditor();
    void toggleObjectsBar();
    void activateObjectsPanel();
    void showObjectsBar(bool show);

private:
    EditorView *editorView_ {nullptr};
    QTreeView *objectTreeView_ {nullptr};
    QWidget *leftContainer_ {nullptr};
    QWidget *activityBar_ {nullptr};
    QToolButton *objectsButton_ {nullptr};
    PanelArea *panelArea_ {nullptr};
    QWidget *objectsPanel_ {nullptr};
    int activityBarFixedWidth_ {36};
    int lastObjectsBarWidth_ {280};
    bool objectsBarVisible_ {true};
};


