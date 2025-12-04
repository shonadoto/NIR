#include "ActivityButton.h"

namespace {
constexpr int kButtonPaddingPx = 8;
}

ActivityButton::ActivityButton(QWidget* parent) : QToolButton(parent) {
  setAutoRaise(true);
}

void ActivityButton::configure(const QIcon& icon, const QSize& iconSize,
                               bool checkable, bool checked) {
  setIcon(icon);
  setIconSize(iconSize);
  setCheckable(checkable);
  setChecked(checked);
  setFixedSize(iconSize.width() + kButtonPaddingPx,
               iconSize.height() + kButtonPaddingPx);
}
