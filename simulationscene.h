#ifndef SIMULATIONSCENE_H
#define SIMULATIONSCENE_H

#include <QGraphicsScene>
#include <QList>
#include <QInputDialog>

#include "damperitem.h"
#include "wallitem.h"

class MassItem;
class SpringItem;

enum class InteractionMode
{
    Select,
    AddSpring,
    AddDamper,
    AddForce
};

class SimulationScene : public QGraphicsScene
{
    Q_OBJECT

signals:
    void springAdded();

public:
    explicit SimulationScene(QObject *parent = nullptr);
    void setMode(InteractionMode mode);
    QList<MassItem *> getMasses() const;
    QList<SpringItem *> getSprings() const { return m_springs; }
    QList<DamperItem *> getDampers() const { return m_dampers; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    InteractionMode m_mode = InteractionMode::Select;
    QGraphicsItem *m_firstSelected = nullptr;

    QList<SpringItem *> m_springs;
    QList<DamperItem *> m_dampers;
    QList<WallItem *> m_walls;
    QVector<MassItem*> m_masses;

    SpringItem *findSpring(QGraphicsItem *a, QGraphicsItem *b) const;
    DamperItem *findDamper(QGraphicsItem *a, QGraphicsItem *b) const;
    void rebalanceOffsets(QGraphicsItem *a, QGraphicsItem *b);

    static constexpr qreal kHalfGap = 20.0;
};

#endif