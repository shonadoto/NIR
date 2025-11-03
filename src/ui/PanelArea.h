#pragma once

#include <QWidget>

class QGridLayout;

class PanelArea : public QWidget {
  Q_OBJECT
public:
  explicit PanelArea(QWidget *parent = nullptr);
  ~PanelArea() override;

  void addPanel(QWidget *panel, int row, int col, int rowSpan = 1,
                int colSpan = 1);
  void setColumnStretch(int column, int stretch);
  void setRowStretch(int row, int stretch);

private:
  QGridLayout *grid_{nullptr};
};
