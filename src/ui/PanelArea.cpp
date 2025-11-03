#include "PanelArea.h"

#include <QGridLayout>

PanelArea::PanelArea(QWidget *parent)
    : QWidget(parent), grid_(new QGridLayout(this)) {
    grid_->setContentsMargins(0, 0, 0, 0);
    grid_->setSpacing(0);
    setLayout(grid_);
}

PanelArea::~PanelArea() = default;

void PanelArea::addPanel(QWidget *panel, int row, int col, int rowSpan, int colSpan) {
    grid_->addWidget(panel, row, col, rowSpan, colSpan);
}

void PanelArea::setColumnStretch(int column, int stretch) {
    grid_->setColumnStretch(column, stretch);
}

void PanelArea::setRowStretch(int row, int stretch) {
    grid_->setRowStretch(row, stretch);
}


