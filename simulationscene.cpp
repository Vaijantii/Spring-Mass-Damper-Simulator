#include "simulationscene.h"
#include "massitem.h"
#include "springitem.h"
#include "forcedialog.h"
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QPushButton>

SimulationScene::SimulationScene(QObject *parent)
    : QGraphicsScene(parent)
{
}

void SimulationScene::setMode(InteractionMode mode)
{
    m_mode = mode;
    m_firstSelected = nullptr;

    for (QGraphicsItem *item : items())
    {
        bool movable = (mode == InteractionMode::Select);
        if (dynamic_cast<MassItem *>(item) || dynamic_cast<WallItem *>(item))
            item->setFlag(QGraphicsItem::ItemIsMovable, movable);
    }
}

SpringItem *SimulationScene::findSpring(QGraphicsItem *a, QGraphicsItem *b) const
{
    for (SpringItem *s : m_springs)
        if ((s->itemA() == a && s->itemB() == b) ||
            (s->itemA() == b && s->itemB() == a))
            return s;
    return nullptr;
}

DamperItem *SimulationScene::findDamper(QGraphicsItem *a, QGraphicsItem *b) const
{
    for (DamperItem *d : m_dampers)
        if ((d->itemA() == a && d->itemB() == b) ||
            (d->itemA() == b && d->itemB() == a))
            return d;
    return nullptr;
}

void SimulationScene::rebalanceOffsets(QGraphicsItem *a, QGraphicsItem *b)
{
    SpringItem *s = findSpring(a, b);
    DamperItem *d = findDamper(a, b);
    if (s && d)
    {
        d->setOffset(kHalfGap);
        s->setOffset(-kHalfGap);
    }
    else if (s)
    {
        s->setOffset(0.0);
    }
    else if (d)
    {
        d->setOffset(0.0);
    }
}

static char askSeriesOrParallel(const QString &sym)
{
    QMessageBox box;
    box.setWindowTitle("Combine components");
    box.setText("A " + sym + " already exists between these two elements.\n"
                             "How do you want to combine them?");
    QPushButton *parallelBtn = box.addButton(
        "Parallel  ( " + sym + "_eq = " + sym + "1 + " + sym + "2 )",
        QMessageBox::AcceptRole);
    QPushButton *seriesBtn = box.addButton(
        "Series    ( 1/" + sym + "_eq = 1/" + sym + "1 + 1/" + sym + "2 )",
        QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() == parallelBtn)
        return 'P';
    if (box.clickedButton() == seriesBtn)
        return 'S';
    return '\0';
}

void SimulationScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Force mode
    if (m_mode == InteractionMode::AddForce)
    {
        QGraphicsItem *raw = itemAt(event->scenePos(), QTransform());
        MassItem *mass = dynamic_cast<MassItem *>(raw);

        if (mass)
        {
            // Pre-populate with existing force if there is one
            ForceFunction current = mass->hasForce() ? mass->forceFunction()
                                                     : ForceFunction{};
            ForceDialog dlg(current);
            if (dlg.exec() == QDialog::Accepted)
                mass->setForce(dlg.result());

            setMode(InteractionMode::Select);
            emit springAdded(); // resets toolbar buttons
        }
        return;
    }

    // Spring / Damper mode
    if (m_mode == InteractionMode::AddSpring ||
        m_mode == InteractionMode::AddDamper)
    {
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        bool isConnectable = dynamic_cast<MassItem *>(item) ||
                             dynamic_cast<WallItem *>(item);

        if (item && isConnectable)
        {
            if (!m_firstSelected)
            {
                m_firstSelected = item;
                if (auto *m = dynamic_cast<MassItem *>(item))
                    m->setHighlighted(true);
                if (auto *w = dynamic_cast<WallItem *>(item))
                    w->setHighlighted(true);
            }
            else if (item != m_firstSelected)
            {
                if (m_mode == InteractionMode::AddSpring)
                {
                    bool ok;
                    double k2 = QInputDialog::getDouble(
                        nullptr, "Spring Constant", "Enter stiffness k (N/m):",
                        1.0, 0.001, 100000.0, 3, &ok);
                    if (ok)
                    {
                        SpringItem *ex = findSpring(m_firstSelected, item);
                        if (ex)
                        {
                            char c = askSeriesOrParallel("k");
                            if (c != '\0')
                            {
                                double k1 = ex->getStiffness();
                                ex->setStiffness(c == 'P' ? k1 + k2 : (k1 * k2) / (k1 + k2));
                            }
                        }
                        else
                        {
                            SpringItem *s = new SpringItem(m_firstSelected, item, k2, 0.0);
                            s->setIndex(m_springs.size());
                            addItem(s);
                            m_springs.append(s);
                            rebalanceOffsets(m_firstSelected, item);
                        }
                    }
                }
                else
                {
                    bool ok;
                    double c2 = QInputDialog::getDouble(
                        nullptr, "Damping Coefficient", "Enter damping c (N·s/m):",
                        1.0, 0.001, 100000.0, 3, &ok);
                    if (ok)
                    {
                        DamperItem *ex = findDamper(m_firstSelected, item);
                        if (ex)
                        {
                            char c = askSeriesOrParallel("c");
                            if (c != '\0')
                            {
                                double c1 = ex->getDamping();
                                ex->setDamping(c == 'P' ? c1 + c2 : (c1 * c2) / (c1 + c2));
                            }
                        }
                        else
                        {
                            DamperItem *d = new DamperItem(m_firstSelected, item, c2, 0.0);
                            d->setIndex(m_dampers.size());
                            addItem(d);
                            m_dampers.append(d);
                            rebalanceOffsets(m_firstSelected, item);
                        }
                    }
                }

                if (auto *m = dynamic_cast<MassItem *>(m_firstSelected))
                    m->setHighlighted(false);
                if (auto *w = dynamic_cast<WallItem *>(m_firstSelected))
                    w->setHighlighted(false);

                m_firstSelected = nullptr;
                setMode(InteractionMode::Select);
                emit springAdded();
            }
        }
        return;
    }

    QGraphicsScene::mousePressEvent(event);
    for (SpringItem *s : m_springs)
        s->updatePosition();
    for (DamperItem *d : m_dampers)
        d->updatePosition();
}

QList<MassItem *> SimulationScene::getMasses() const
{
    QList<MassItem *> masses;

    for (QGraphicsItem *item : items())
    {
        if (MassItem *m = dynamic_cast<MassItem *>(item))
        {
            masses.append(m);
        }
    }

    // enforce physical ordering (LEFT → RIGHT)
    std::sort(masses.begin(), masses.end(), [](MassItem *a, MassItem *b)
              { return a->pos().x() < b->pos().x(); });

    return masses;
}