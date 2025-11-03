#pragma once

#include <QGraphicsItem>

class SubstrateItem : public QGraphicsItem {
public:
    explicit SubstrateItem(const QSizeF &size);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QSizeF size() const { return size_; }
    void set_size(const QSizeF &size);

private:
    QSizeF size_;
};


