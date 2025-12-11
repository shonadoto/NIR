#include "ActivityButton.h"

namespace {
constexpr int kButtonPaddingPx = 8;
}

ActivityButton::ActivityButton(QWidget* parent) : QToolButton(parent) {
  setAutoRaise(true);
}

void ActivityButton::configure(const QIcon& icon, const QSize& icon_size,
                               bool checkable, bool checked) {
  setIcon(icon);
  setIconSize(icon_size);
  setCheckable(checkable);
  setChecked(checked);
  setFixedSize(icon_size.width() + kButtonPaddingPx,
               icon_size.height() + kButtonPaddingPx);
}
