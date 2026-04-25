#include "damperitem.h"
#include <cmath>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QFont>
#include <QString>

DamperItem::DamperItem(QGraphicsItem *itemA, QGraphicsItem *itemB,
                       double damping, qreal offset, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_itemA(itemA), m_itemB(itemB),
      m_damping(damping), offset(offset)
{
    setZValue(-1);
    updatePosition();
}

double DamperItem::getDamping() const { return m_damping; }

void DamperItem::updatePosition()
{
    prepareGeometryChange();
    update();
}

QRectF DamperItem::boundingRect() const
{
    if (!m_itemA || !m_itemB)
        return QRectF();
    QPointF a = m_itemA->scenePos();
    QPointF b = m_itemB->scenePos();
    return QRectF(
        QPointF(qMin(a.x(), b.x()) - 20, qMin(a.y(), b.y()) - 20),
        QPointF(qMax(a.x(), b.x()) + 20, qMax(a.y(), b.y()) + 20));
}

void DamperItem::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *,
                       QWidget *)
{
    if (!m_itemA || !m_itemB)
        return;

    QPointF a = m_itemA->scenePos();
    QPointF b = m_itemB->scenePos();

    QPainterPath path = buildDamperPath(a, b);
    painter->setPen(QPen(QColor("#f38ba8"), 2.5, Qt::SolidLine, Qt::RoundCap));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);

    // Label: placed perpendicular to the line, offset away from it
    QPointF delta = b - a;
    qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    QPointF perp(0.0, -1.0);
    if (length > 1e-3)
    {
        QPointF unit = delta / length;
        perp = QPointF(-unit.y(), unit.x());
    }
    QPointF mid = (a + b) / 2.0;
    QPointF labelPos = mid + perp * (offset + 14.0);

    painter->setPen(Qt::white);
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(labelPos,
                      QString("c%1 = %2 N.s/m")
                          .arg(m_index + 1)
                          .arg(QString::number(m_damping, 'g', 4)));
    painter->setFont(QFont("Arial", 7));
    painter->setPen(QColor(220, 180, 180));
}

QPainterPath DamperItem::buildDamperPath(QPointF from, QPointF to) const
{
    QPainterPath path;

    QPointF delta = to - from;
    qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    if (length < 1e-3)
        return path;

    QPointF unit = delta / length;
    QPointF perp = QPointF(-unit.y(), unit.x());

    from = from + perp * offset;
    to = to + perp * offset;

    qreal startGap = 15.0;
    qreal endGap = 15.0;
    qreal boxLen = 30.0;
    qreal boxHalf = 12.0;
    qreal midPoint = length / 2.0;

    // Entry line
    path.moveTo(from);
    path.lineTo(from + unit * startGap);
    path.lineTo(from + unit * (midPoint - boxLen / 2.0));

    // Dashpot box
    QPointF boxStart = from + unit * (midPoint - boxLen / 2.0);
    QPointF boxEnd = from + unit * (midPoint + boxLen / 2.0);
    path.moveTo(boxStart + perp * (-boxHalf));
    path.lineTo(boxEnd + perp * (-boxHalf));
    path.lineTo(boxEnd + perp * (boxHalf));
    path.lineTo(boxStart + perp * (boxHalf));
    path.lineTo(boxStart + perp * (-boxHalf));

    // Piston line
    path.moveTo(from + unit * (midPoint - boxLen / 2.0 + 5.0));
    path.lineTo(from + unit * (midPoint + boxLen / 2.0));

    // Exit line
    path.moveTo(boxEnd);
    path.lineTo(to - unit * endGap);
    path.lineTo(to);

    return path;
}