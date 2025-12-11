#include "SubstrateDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace {
constexpr double kMinSizePx = 10.0;
constexpr double kMaxSizePx = 100000.0;
constexpr double kStepPx = 10.0;
}  // namespace

SubstrateDialog::SubstrateDialog(QWidget* parent, double width_px,
                                 double height_px)
    : QDialog(parent),
      w_spin_(new QDoubleSpinBox(this)),
      h_spin_(new QDoubleSpinBox(this)) {
  setWindowTitle("Substrate Size");

  auto* form = new QFormLayout();
  w_spin_->setRange(kMinSizePx, kMaxSizePx);
  w_spin_->setSingleStep(kStepPx);
  w_spin_->setDecimals(1);
  w_spin_->setValue(width_px);

  h_spin_->setRange(kMinSizePx, kMaxSizePx);
  h_spin_->setSingleStep(kStepPx);
  h_spin_->setDecimals(1);
  h_spin_->setValue(height_px);

  form->addRow("Width (px)", w_spin_);
  form->addRow("Height (px)", h_spin_);

  auto* buttons =
    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(buttons);
}

auto SubstrateDialog::width_px() const -> double {
  return w_spin_->value();
}
auto SubstrateDialog::height_px() const -> double {
  return h_spin_->value();
}
