#include "wallitem.h"
#include "springitem.h"
#include "damperitem.h"
#include <QGraphicsScene>

WallItem::WallItem(WallType type, qreal size, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_type(type), m_size(size)
{
    // Thickness differs per type: Ground is thicker to look like a floor
    switch (m_type) {
    case WallType::Vertical:    m_thickness = 18.0; break;
    case WallType::Horizontal:  m_thickness = 18.0; break;
    case WallType::Ground:      m_thickness = 22.0; break;
    }

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

// ── Bounding rect ────────────────────────────────────────────────────────────
QRectF WallItem::boundingRect() const
{
    switch (m_type) {
    case WallType::Vertical:
        // thin in X, tall in Y
        return QRectF(-m_thickness / 2, -m_size / 2, m_thickness, m_size);

    case WallType::Horizontal:
    case WallType::Ground:
        // wide in X, thin in Y
        return QRectF(-m_size / 2, -m_thickness / 2, m_size, m_thickness);
    }
    return {};
}

// ── itemChange: notify springs/dampers when moved ────────────────────────────
QVariant WallItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene()) {
        for (QGraphicsItem *item : scene()->items()) {
            if (auto *spring = dynamic_cast<SpringItem*>(item))
                spring->updatePosition();
            if (auto *damper = dynamic_cast<DamperItem*>(item))
                damper->updatePosition();
        }
        scene()->update();
    }
    return QGraphicsItem::itemChange(change, value);
}

// ── Highlighted setter ───────────────────────────────────────────────────────
void WallItem::setHighlighted(bool on)
{
    m_highlighted = on;
    update();
}

// ── Paint dispatch ───────────────────────────────────────────────────────────
void WallItem::paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *,
                     QWidget *)
{
    switch (m_type) {
    case WallType::Vertical:    paintVertical(painter);   break;
    case WallType::Horizontal:  paintHorizontal(painter); break;
    case WallType::Ground:      paintGround(painter);     break;
    }
}

// ── Vertical wall  (original look, hatches lean right) ───────────────────────
void WallItem::paintVertical(QPainter *painter) const
{
    QRectF r = boundingRect();

    // Fill
    QColor fill = m_highlighted ? QColor("#fab387") : QColor("#45475a");
    painter->setBrush(fill);
    painter->setPen(QPen(Qt::white, 1.5));
    painter->drawRect(r);

    // Diagonal hatches leaning to the right
    painter->setPen(QPen(QColor("#6c7086"), 1.2));
    int steps = (int)(m_size / 12);
    for (int i = 0; i <= steps; ++i) {
        qreal y = r.top() + i * (m_size / steps);
        painter->drawLine(QPointF(r.left(), y), QPointF(r.right(), y + 10));
    }

    // Label
    painter->setPen(Qt::white);
    painter->setFont(QFont("Arial", 8, QFont::Bold));
    painter->drawText(r, Qt::AlignCenter, "W");
}

// ── Horizontal wall  (rotated version: hatches lean downward) ────────────────
void WallItem::paintHorizontal(QPainter *painter) const
{
    QRectF r = boundingRect();

    QColor fill = m_highlighted ? QColor("#fab387") : QColor("#45475a");
    painter->setBrush(fill);
    painter->setPen(QPen(Qt::white, 1.5));
    painter->drawRect(r);

    // Diagonal hatches leaning downward
    painter->setPen(QPen(QColor("#6c7086"), 1.2));
    int steps = (int)(m_size / 12);
    for (int i = 0; i <= steps; ++i) {
        qreal x = r.left() + i * (m_size / steps);
        painter->drawLine(QPointF(x, r.top()), QPointF(x + 10, r.bottom()));
    }

    // Label
    painter->setPen(Qt::white);
    painter->setFont(QFont("Arial", 8, QFont::Bold));
    painter->drawText(r, Qt::AlignCenter, "W");
}

// ── Ground  (wider slab, heavier hatches, ground symbol below) ───────────────
void WallItem::paintGround(QPainter *painter) const
{
    QRectF r = boundingRect();

    // Main slab – slightly different colour to distinguish from plain wall
    QColor fill = m_highlighted ? QColor("#fab387") : QColor("#313244");
    painter->setBrush(fill);
    painter->setPen(QPen(QColor("#cdd6f4"), 1.8));
    painter->drawRect(r);

    // Dense downward hatches
    painter->setPen(QPen(QColor("#6c7086"), 1.2));
    int steps = (int)(m_size / 10);
    for (int i = 0; i <= steps; ++i) {
        qreal x = r.left() + i * (m_size / steps);
        painter->drawLine(QPointF(x, r.top()), QPointF(x + 12, r.bottom()));
    }

    // Ground symbol: three horizontal lines of decreasing length below the slab
    painter->setPen(QPen(QColor("#cdd6f4"), 1.8));
    qreal cx = 0.0;                     // item-local centre X
    qreal y0 = r.bottom() + 5.0;
    const qreal lengths[] = { 40.0, 26.0, 14.0 };
    const qreal gap       = 5.0;
    for (int i = 0; i < 3; ++i) {
        qreal half = lengths[i] / 2.0;
        qreal y    = y0 + i * gap;
        painter->drawLine(QPointF(cx - half, y), QPointF(cx + half, y));
    }

    // Label
    painter->setPen(QColor("#cdd6f4"));
    painter->setFont(QFont("Arial", 8, QFont::Bold));
    painter->drawText(r, Qt::AlignCenter, "GND");
}