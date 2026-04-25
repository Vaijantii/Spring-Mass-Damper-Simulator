#ifndef WALLITEM_H
#define WALLITEM_H

#include <QGraphicsItem>
#include <QPainter>

class WallItem : public QGraphicsItem
{
public:
    enum class WallType {
        Vertical,    // original: thin vertical slab, hatches go right
        Horizontal,  // thin horizontal slab, hatches go down
        Ground       // wide horizontal base, hatches go down (fixed floor)
    };

    // 'size' means height for Vertical, width for Horizontal/Ground
    explicit WallItem(WallType type = WallType::Vertical,
                      qreal size = 120.0,
                      QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void   paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    void     setHighlighted(bool on);
    WallType wallType() const { return m_type; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    WallType m_type;
    qreal    m_size;          // height (Vertical) or width (Horizontal/Ground)
    qreal    m_thickness;     // the thin dimension
    bool     m_highlighted = false;

    void paintVertical  (QPainter *p) const;
    void paintHorizontal(QPainter *p) const;
    void paintGround    (QPainter *p) const;
};

#endif