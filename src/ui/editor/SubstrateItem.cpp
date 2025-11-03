#include "SubstrateItem.h"

#include <QPainter>

namespace {
constexpr qreal kCornerRadiusPx = 8.0;
constexpr qreal kOutlineWidthPx = 1.0;
}

SubstrateItem::SubstrateItem(const QSizeF &size)
    : size_(size) {
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setZValue(-100.0); // stay behind other shapes later
}

QRectF SubstrateItem::boundingRect() const {
    return QRectF(QPointF(0, 0), size_);
}

void SubstrateItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QRectF rect = boundingRect();

    QPainterPath path;
    path.addRoundedRect(rect.adjusted(kOutlineWidthPx, kOutlineWidthPx,
                                      -kOutlineWidthPx, -kOutlineWidthPx),
                        kCornerRadiusPx, kCornerRadiusPx);

    painter->fillPath(path, QColor(240, 240, 240));
    QPen pen(QColor(180, 180, 180));
    pen.setWidthF(kOutlineWidthPx);
    painter->setPen(pen);
    painter->drawPath(path);
}


