#ifndef DAMPERITEM_H
#define DAMPERITEM_H

#include <QGraphicsItem>
#include <QPainter>

class DamperItem : public QGraphicsItem
{
public:
    DamperItem(QGraphicsItem *itemA, QGraphicsItem *itemB,
               double damping = 1.0, qreal offset = 0.0,
               QGraphicsItem *parent = nullptr);

    void updatePosition();

    QRectF boundingRect() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    double getDamping() const;
    void setDamping(double c)
    {
        m_damping = c;
        update();
    }
    void setIndex(int idx)
    {
        m_index = idx;
        update();
    }
    int index() const { return m_index; }

    QGraphicsItem *itemA() const { return m_itemA; }
    QGraphicsItem *itemB() const { return m_itemB; }
    void setOffset(qreal o)
    {
        offset = o;
        updatePosition();
    }

private:
    QGraphicsItem *m_itemA;
    QGraphicsItem *m_itemB;
    double m_damping;
    qreal offset;
    int m_index = 0;

    QPainterPath buildDamperPath(QPointF from, QPointF to) const;
};

#endif