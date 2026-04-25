#include "massitem.h"
#include "springitem.h"
#include "damperitem.h"
#include <QGraphicsScene>
#include <cmath>

static constexpr double DEG2RAD = M_PI / 180.0;

MassItem::MassItem(double mass, qreal width, qreal height, QGraphicsItem *parent)
    : QGraphicsItem(parent), w(width), h(height), mass(mass)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

double MassItem::getMass() const { return mass; }

void MassItem::setHighlighted(bool on)
{
    m_highlighted = on;
    update();
}

void MassItem::setForce(const ForceFunction &fn)
{
    m_hasForce = true;
    m_force = fn;
    prepareGeometryChange();
    update();
}

void MassItem::clearForce()
{
    m_hasForce = false;
    prepareGeometryChange();
    update();
}

QRectF MassItem::boundingRect() const
{
    qreal extra = m_hasForce ? 100.0 : 0.0;
    return QRectF(-w / 2 - extra, -h / 2 - extra, w + 2 * extra, h + 2 * extra);
}

QVariant MassItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene())
    {
        for (QGraphicsItem *item : scene()->items())
        {
            if (SpringItem *spring = dynamic_cast<SpringItem *>(item))
                spring->updatePosition();
            if (DamperItem *damper = dynamic_cast<DamperItem *>(item))
                damper->updatePosition();
        }
        scene()->update();
    }
    return QGraphicsItem::itemChange(change, value);
}

void MassItem::paintForceArrow(QPainter *painter) const
{
    if (!m_hasForce)
        return;

    double angle = m_force.angleDeg();
    double rad = angle * DEG2RAD;

    // Direction unit vector (math convention: 90° = up, negate Y for screen)
    QPointF dir(std::cos(rad), -std::sin(rad));
    QPointF perp(-dir.y(), dir.x());

    // Arrow length: fixed 60px (direction is the meaningful info; magnitude is in F(t))
    const qreal arrowLen = 60.0;
    const qreal headLen = 13.0;
    const qreal headWid = 6.0;

    // Start at the mass boundary edge in the arrow direction
    qreal ex = (w / 2) * std::abs(dir.x());
    qreal ey = (h / 2) * std::abs(dir.y());
    QPointF origin = dir * std::sqrt(ex * ex + ey * ey);
    QPointF tip = origin + dir * arrowLen;

    // Shaft
    painter->setPen(QPen(QColor("#f9e2af"), 2.5, Qt::SolidLine, Qt::RoundCap));
    painter->drawLine(origin, tip);

    // Arrowhead
    QPolygonF head;
    head << tip
         << (tip - dir * headLen + perp * headWid)
         << (tip - dir * headLen - perp * headWid);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#f9e2af"));
    painter->drawPolygon(head);

    // Label: show the expression string
    painter->setPen(QColor("#f9e2af"));
    painter->setFont(QFont("Courier New", 8, QFont::Bold));
    QString label = "F(t) = " + m_force.expression();
    if (m_force.omega() != 1.0)
        label += QString("  (ω=%1)").arg(m_force.omega(), 0, 'g', 4);
    painter->drawText(tip + dir * 5.0 + perp * 4.0, label);

    // Direction tag below label
    painter->setFont(QFont("Arial", 7));
    painter->setPen(QColor("#a6adc8"));
    painter->drawText(tip + dir * 5.0 - perp * 10.0,
                      QString("@ %1°").arg(angle, 0, 'f', 1));
}

void MassItem::paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *,
                     QWidget *)
{
    // Shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 60));
    painter->drawRect(QRectF(-w / 2 + 4, -h / 2 + 4, w, h));

    // Fill
    QColor fillColor;
    if (m_highlighted)
        fillColor = QColor("#fab387");
    else if (isSelected())
        fillColor = QColor("#89b4fa");
    else
        fillColor = QColor("#cba6f7");

    painter->setBrush(fillColor);
    painter->setPen(QPen(Qt::white, 1.5));
    painter->drawRect(QRectF(-w / 2, -h / 2, w, h));

    // Mass label — "m1" on top, kg value below
    painter->setPen(Qt::white);
    painter->setFont(QFont("Arial", 11, QFont::Bold));
    painter->drawText(QRectF(-w / 2, -h / 2, w, h * 0.55), Qt::AlignCenter,
                      QString("m%1").arg(m_index + 1));

    painter->setFont(QFont("Arial", 8));
    painter->setPen(QColor(220, 220, 220));
    painter->drawText(QRectF(-w / 2, -h / 2 + h * 0.5, w, h * 0.5), Qt::AlignCenter,
                      QString::number(mass, 'g', 4) + " kg");

    // Force arrow
    paintForceArrow(painter);
}