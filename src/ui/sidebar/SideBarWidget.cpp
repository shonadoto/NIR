#include "SideBarWidget.h"

#include <ui/activity/ActivityBar.h>
#include <ui/activity/ActivityButton.h>
#include <ui/panels/ObjectsBar.h>

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QString>

namespace {
constexpr int kDefaultObjectsBarWidthPx = 280;
}  // namespace

SideBarWidget::SideBarWidget(QWidget* parent)
    : QWidget(parent), activity_bar_(this), stack_(this), layout_(this) {
  layout_.setContentsMargins(0, 0, 0, 0);
  layout_.setSpacing(0);

  layout_.addWidget(&activity_bar_);

  stack_.setVisible(false);
  layout_.addWidget(&stack_, 1);

  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setFixedWidth(activity_bar_.sizeHint().width());
}

SideBarWidget::~SideBarWidget() = default;

void SideBarWidget::registerSidebar(const QString& sidebar_id,
                                    const QIcon& icon, QWidget* content,
                                    int preferredWidth) {
  Entry entry;
  entry.id = sidebar_id;
  entry.content = content;
  entry.stack_index = stack_.addWidget(content);
  entry.preferred_width = preferredWidth;

  auto* btn = activity_bar_.addToggleButton(icon, QString(), false);
  QObject::connect(
    btn, &QToolButton::toggled, this,
    [this, entry_id = entry.id](bool /*on*/) { setActive(entry_id); });
  entry.button = btn;
  id_to_entry_.insert(sidebar_id, std::move(entry));

  // Ensure initial width accounts for new button's sizeHint
  applyWidth();
}

void SideBarWidget::registerObjectBar() {
  registerSidebar("objects", QIcon(":/icons/objects.svg"), new ObjectsBar(this),
                  kDefaultObjectsBarWidthPx);
}

void SideBarWidget::setActive(const QString& entry_id) {
  if (current_id_ == entry_id) {
    // Collapse
    current_id_ = std::nullopt;
    stack_.setVisible(false);

    applyWidth();
    return;
  }

  if (current_id_.has_value()) {
    auto& prev = id_to_entry_[current_id_.value()];
    if (prev.button != nullptr) {
      prev.button->setChecked(false);
    }
    prev.last_width = stack_.isVisible() ? stack_.width() : prev.last_width;
  }

  current_id_ = entry_id;

  if (!id_to_entry_.contains(entry_id)) {
    throw std::runtime_error(QString("SideBarWidget: Entry not found: %1")
                               .arg(entry_id)
                               .toStdString());
  }

  auto& entry = id_to_entry_[entry_id];
  if (entry.button != nullptr && !entry.button->isChecked()) {
    entry.button->setChecked(true);
  }

  stack_.setCurrentIndex(entry.stack_index);
  stack_.setVisible(true);

  applyWidth();
}

void SideBarWidget::applyWidth() {
  int stackWidth = 0;
  if (current_id_.has_value()) {
    if (!id_to_entry_.contains(current_id_.value())) {
      throw std::runtime_error(QString("SideBarWidget: Entry not found: %1")
                                 .arg(current_id_.value())
                                 .toStdString());
    }
    auto& entry = id_to_entry_[current_id_.value()];
    stackWidth =
      entry.last_width > 0 ? entry.last_width : entry.preferred_width;
    stack_.setFixedWidth(stackWidth);
  }
  const int barW = activity_bar_.sizeHint().width();
  setFixedWidth(barW + stackWidth);
}