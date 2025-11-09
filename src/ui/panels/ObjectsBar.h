#pragma once

#include <QWidget>
#include <QAbstractItemModel>

class QTreeView;

class ObjectsBar : public QWidget {
    Q_OBJECT
public:
    explicit ObjectsBar(QWidget *parent = nullptr);
    ~ObjectsBar() override;

    QTreeView* treeView() const { return tree_view_; }
    int preferredWidth() const { return preferred_width_; }
    void setPreferredWidth(int w);
    void set_model(QAbstractItemModel *model);

private:
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void setActive(bool visible) { setVisible(visible); }
    void toggle() { setVisible(!isVisible()); }

private:
    QTreeView *tree_view_ {nullptr};
    int preferred_width_;
    int last_visible_width_;

protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
};


