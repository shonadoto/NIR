#include "ActivityBar.h"
#include "ActivityButton.h"

#include <QVBoxLayout>

namespace {
constexpr int kActivityBarMarginPx = 6;
constexpr int kInitialActivityBarWidthPx = 44;
constexpr int kDefaultIconSizePx = 24;
}

ActivityBar::ActivityBar(QWidget *parent)
    : QWidget(parent), layout_(new QVBoxLayout(this)), margin_(kActivityBarMarginPx), fixed_width_(kInitialActivityBarWidthPx) {
    layout_->setContentsMargins(margin_, margin_, margin_, margin_);
    layout_->setSpacing(margin_);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(fixed_width_);
}

ActivityBar::~ActivityBar() = default;

std::shared_ptr<ActivityButton> ActivityBar::addToggleButton(const QIcon &icon, const QString &text, bool checked) {
    auto btn = std::make_shared<ActivityButton>(this);
    btn->setToolTip(text);
    btn->configure(icon, QSize(kDefaultIconSizePx, kDefaultIconSizePx), true, checked);
    layout_->addWidget(btn.get(), 0, Qt::AlignTop);

    const int w = btn->sizeHint().width() + 2 * margin_;
    if (w > fixed_width_) {
        fixed_width_ = w;
        setFixedWidth(fixed_width_);
    }
    return btn;
}

// No external margin control currently; keep default margins


