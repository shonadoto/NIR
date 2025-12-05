#include "ActivityBar.h"

#include <QVBoxLayout>

#include "ActivityButton.h"

namespace {
constexpr int kActivityBarMarginPx = 6;
constexpr int kInitialActivityBarWidthPx = 44;
constexpr int kDefaultIconSizePx = 24;
}  // namespace

ActivityBar::ActivityBar(QWidget* parent)
    : QWidget(parent),
      layout_(new QVBoxLayout(this)),
      margin_(kActivityBarMarginPx),
      fixed_width_(kInitialActivityBarWidthPx) {
  layout_->setContentsMargins(margin_, margin_, margin_, margin_);
  layout_->setSpacing(margin_);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setFixedWidth(fixed_width_);
}

ActivityBar::~ActivityBar() = default;

ActivityButton* ActivityBar::addToggleButton(const QIcon& icon,
                                             const QString& text,
                                             bool checked) {
  // Qt uses parent-based ownership, not smart pointers
  auto* btn = new ActivityButton(this);
  btn->setToolTip(text);
  btn->configure(icon, QSize(kDefaultIconSizePx, kDefaultIconSizePx), true,
                 checked);
  layout_->addWidget(btn, 0, Qt::AlignTop);

  const int button_width = btn->sizeHint().width() + 2 * margin_;
  if (button_width > fixed_width_) {
    fixed_width_ = button_width;
    setFixedWidth(fixed_width_);
  }
  return btn;
}

// No external margin control currently; keep default margins
