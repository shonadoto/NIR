#pragma once

#include <QDialog>

class QDoubleSpinBox;

class SubstrateDialog : public QDialog {
  Q_OBJECT
 public:
  explicit SubstrateDialog(QWidget* parent, double width_px, double height_px);
  ~SubstrateDialog() override = default;

  double width_px() const;
  double height_px() const;

 private:
  QDoubleSpinBox* w_spin_{nullptr};
  QDoubleSpinBox* h_spin_{nullptr};
};
