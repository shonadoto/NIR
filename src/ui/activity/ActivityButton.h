#pragma once

#include <QToolButton>

class ActivityButton : public QToolButton {
  Q_OBJECT
 public:
  explicit ActivityButton(QWidget* parent = nullptr);
  void configure(const QIcon& icon, const QSize& icon_size,
                 bool checkable = true, bool checked = true);
};
