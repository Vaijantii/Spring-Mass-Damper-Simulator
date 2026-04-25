#ifndef SPRINGITEM_H
#define SPRINGITEM_H

#include <QGraphicsItem>
#include <QPainter>

class SpringItem : public QGraphicsItem
{
public:
    SpringItem(QGraphicsItem *itemA, QGraphicsItem *itemB,
               double stiffness = 1.0, qreal offset = 0.0,
               QGraphicsItem *parent = nullptr);

    void updatePosition();

    QRectF boundingRect() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    double getStiffness() const;
    void setStiffness(double k)
    {
        stiffness = k;
        update();
    }
    void setIndex(int idx)
    {
        m_index = idx;
        update();
    }
    int index() const { return m_index; }

    QGraphicsItem *itemA() const { return m_massA; }
    QGraphicsItem *itemB() const { return m_massB; }
    void setOffset(qreal o)
    {
        offset = o;
        updatePosition();
    }

private:
    QGraphicsItem *m_massA;
    QGraphicsItem *m_massB;
    double stiffness;
    qreal offset;
    int m_index = 0;

    QPainterPath buildSpringPath(QPointF from, QPointF to) const;
};

#endif