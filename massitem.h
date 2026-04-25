#ifndef MASSITEM_H
#define MASSITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include "forcefunction.h"

class SpringItem;
class DamperItem;

class MassItem : public QGraphicsItem
{
public:
    MassItem(double mass = 1.0, qreal width = 60, qreal height = 60,
             QGraphicsItem *parent = nullptr);

    void setHighlighted(bool on);

    // Force function (arbitrary F(t), direction angle)
    void setForce(const ForceFunction &fn);
    void clearForce();
    bool hasForce() const { return m_hasForce; }
    const ForceFunction &forceFunction() const { return m_force; }

    double getMass() const;
    void setIndex(int idx)
    {
        m_index = idx;
        update();
    }
    int index() const { return m_index; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change,
                        const QVariant &value) override;

private:
    qreal w, h;
    bool m_highlighted = false;
    double mass;
    int m_index = 0;

    bool m_hasForce = false;
    ForceFunction m_force;

    void paintForceArrow(QPainter *painter) const;
};

#endif