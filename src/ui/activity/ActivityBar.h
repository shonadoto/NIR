#pragma once

#include <QWidget>
class QVBoxLayout;

class QVBoxLayout;
class ActivityButton;

class ActivityBar : public QWidget {
    Q_OBJECT
public:
    explicit ActivityBar(QWidget *parent = nullptr);
    ~ActivityBar() override;

    ActivityButton* addToggleButton(const QIcon &icon, const QString &text, bool checked = false);

private:
    QVBoxLayout *layout_ {nullptr};
    int margin_ {0};
    int fixed_width_ {0};
};


