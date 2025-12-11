#pragma once

#include <QHBoxLayout>
#include <QMap>
#include <QPointer>
#include <QStackedWidget>
#include <QWidget>
#include <optional>

#include "ui/activity/ActivityBar.h"
class ActivityButton;

class SideBarWidget : public QWidget {
  Q_OBJECT
 public:
  explicit SideBarWidget(QWidget* parent = nullptr);
  ~SideBarWidget() override;

  void registerSidebar(const QString& sidebar_id, const QIcon& icon,
                       QWidget* content, int preferredWidth = 280);

 private:
  struct Entry {
    QString id;
    QPointer<ActivityButton> button;
    QPointer<QWidget> content;
    int stack_index{-1};
    int preferred_width{280};
    int last_width{0};
  };

  void setActive(const QString& entry_id);
  void applyWidth();

 private:
  void registerObjectBar();

 private:
  ActivityBar activity_bar_;
  QStackedWidget stack_;
  QHBoxLayout layout_;
  QMap<QString, Entry> id_to_entry_;
  std::optional<QString> current_id_;
};
