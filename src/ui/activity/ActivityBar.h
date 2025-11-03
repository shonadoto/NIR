#pragma once

#include <QWidget>
#include <memory>

class QVBoxLayout;
class ActivityButton;

class ActivityBar : public QWidget {
    Q_OBJECT
public:
    explicit ActivityBar(QWidget *parent = nullptr);
    ~ActivityBar() override;

    std::shared_ptr<ActivityButton> addToggleButton(const QIcon &icon, const QString &text, bool checked = false);

private:
    std::unique_ptr<QVBoxLayout> layout_;
    int margin_;
    int fixed_width_;
};


