#include "springitem.h"
#include "massitem.h"
#include <QGraphicsScene>
#include <cmath>

SpringItem::SpringItem(QGraphicsItem *itemA, QGraphicsItem *itemB,
                       double stiffness, qreal offset, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_massA(itemA), m_massB(itemB),
      stiffness(stiffness), offset(offset)
{
    setZValue(-1);
    updatePosition();
}

double SpringItem::getStiffness() const { return stiffness; }

void SpringItem::updatePosition()
{
    prepareGeometryChange();
    update();
}

QRectF SpringItem::boundingRect() const
{
    if (!m_massA || !m_massB)
        return QRectF();
    QPointF a = m_massA->scenePos();
    QPointF b = m_massB->scenePos();
    return QRectF(
        QPointF(qMin(a.x(), b.x()) - 20, qMin(a.y(), b.y()) - 20),
        QPointF(qMax(a.x(), b.x()) + 20, qMax(a.y(), b.y()) + 20));
}

void SpringItem::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *,
                       QWidget *)
{
    if (!m_massA || !m_massB)
        return;

    QPointF a = m_massA->scenePos();
    QPointF b = m_massB->scenePos();

    QPainterPath path = buildSpringPath(a, b);
    painter->setPen(QPen(QColor("#a6e3a1"), 2.5, Qt::SolidLine, Qt::RoundCap));
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
    QPointF labelPos = mid + perp * (offset - 20.0);

    painter->setPen(Qt::white);
    painter->setFont(QFont("Arial", 9, QFont::Bold));

    QString text = QString("k%1 = %2 N/m")
                       .arg(m_index + 1)
                       .arg(stiffness, 0, 'g', 4);

    // center align
    QFontMetrics fm(painter->font());
    QPointF alignedPos = labelPos - QPointF(fm.horizontalAdvance(text) / 2.0, 0);

    painter->drawText(alignedPos, text);
}

QPainterPath SpringItem::buildSpringPath(QPointF from, QPointF to) const
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

    int coils = 8;
    qreal amplitude = 10.0;
    qreal startGap = 15.0;
    qreal endGap = 15.0;

    qreal coilLength = length - startGap - endGap;
    if (coilLength < 1.0)
        coilLength = 1.0;

    path.moveTo(from);
    path.lineTo(from + unit * startGap);

    int totalSteps = coils * 2;
    for (int i = 0; i <= totalSteps; ++i)
    {
        qreal t = (qreal)i / totalSteps;
        qreal along = startGap + t * coilLength;
        qreal side = (i % 2 == 0) ? amplitude : -amplitude;
        path.lineTo(from + unit * along + perp * side);
    }

    path.lineTo(to);
    return path;
}