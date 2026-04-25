#ifndef SYSTEMBUILDER_H
#define SYSTEMBUILDER_H

#include <QList>
#include <QVector>
#include <QMap>
#include <QString>
#include <QGraphicsItem>
#include <cmath>

#include "massitem.h"
#include "springitem.h"
#include "damperitem.h"
#include "wallitem.h"

// ---------------------------------------------------------------------------
// SystemMatrices (1D Version)
//   All matrices are stored row-major in flat QVector<double> of size n*n.
//   DOF layout: mass_0 → DOF 0 (x), mass_1 → DOF 1 (x), etc.
// ---------------------------------------------------------------------------
struct SystemMatrices
{
    int n = 0; // total DOF count (1 x number of masses)
    int numMasses = 0;

    // Flat row-major n×n matrices
    QVector<double> M; // mass matrix (diagonal)
    QVector<double> K; // stiffness matrix
    QVector<double> C; // damping matrix

    // Initial state vectors (length n)
    QVector<double> x0; // initial displacements
    QVector<double> v0; // initial velocities

    // Maps a MassItem pointer → its DOF index
    QMap<const MassItem *, int> dofIndex;

    // Human-readable label for each DOF
    QVector<QString> dofLabels;

    // Inline element access helpers
    double &M_at(int r, int c) { return M[r * n + c]; }
    double &K_at(int r, int c) { return K[r * n + c]; }
    double &C_at(int r, int c) { return C[r * n + c]; }
    double M_at(int r, int c) const { return M[r * n + c]; }
    double K_at(int r, int c) const { return K[r * n + c]; }
    double C_at(int r, int c) const { return C[r * n + c]; }

    struct ForceTerm
    {
        int dof;
        double scale; // cos(angle) for x-DOF
        const ForceFunction *fn;
    };
    QVector<ForceTerm> forceTerms;

    QVector<double> evalF(double t) const
    {
        QVector<double> F(n, 0.0);
        for (const ForceTerm &ft : forceTerms)
        {
            double mag = ft.fn->evaluate(t);
            F[ft.dof] += ft.scale * mag;
        }
        return F;
    }

    bool isValid() const { return n > 0; }
};

// ---------------------------------------------------------------------------
// SystemBuilder (1D Version)
// ---------------------------------------------------------------------------
class SystemBuilder
{
public:
    static SystemMatrices build(const QList<MassItem *> &masses,
                                const QList<SpringItem *> &springs,
                                const QList<DamperItem *> &dampers)
    {
        SystemMatrices sys;

        // ── 1. Assign DOFs (1 per mass) ─────────────────────────────────────
        sys.numMasses = masses.size();
        sys.n = sys.numMasses;

        if (sys.n == 0)
            return sys; // nothing to build

        sys.M.fill(0.0, sys.n * sys.n);
        sys.K.fill(0.0, sys.n * sys.n);
        sys.C.fill(0.0, sys.n * sys.n);
        sys.x0.fill(0.0, sys.n);
        sys.v0.fill(0.0, sys.n);

        for (int i = 0; i < masses.size(); ++i)
        {
            const MassItem *m = masses[i];
            sys.dofIndex[m] = i;
            sys.dofLabels.append(QString("m%1_x").arg(i + 1));
        }

        // ── 2. Mass matrix (diagonal) ────────────────────────────────────────
        for (int i = 0; i < masses.size(); ++i)
        {
            double m = masses[i]->getMass();
            sys.M_at(i, i) = m;
        }

        // ── 3. Springs → K matrix ────────────────────────────────────────────
        for (const SpringItem *s : springs)
            assembleConnector(sys, s->itemA(), s->itemB(), s->getStiffness(), sys.K);

        // ── 4. Dampers → C matrix ────────────────────────────────────────────
        for (const DamperItem *d : dampers)
            assembleConnector(sys, d->itemA(), d->itemB(), d->getDamping(), sys.C);

        // ── 5. Force terms (X-axis projection only) ──────────────────────────
        for (int i = 0; i < masses.size(); ++i)
        {
            const MassItem *m = masses[i];
            if (!m->hasForce())
                continue;

            const ForceFunction &fn = m->forceFunction();
            int di = sys.dofIndex[m];

            // Project force onto X-axis using cos(angle)
            double rad = fn.angleDeg() * M_PI / 180.0;
            double cos_a = std::cos(rad);

            if (std::abs(cos_a) > 1e-12)
                sys.forceTerms.append({di, cos_a, &fn});
        }

        return sys;
    }

private:
    static bool nodeInfo(const QGraphicsItem *item,
                         const SystemMatrices &sys,
                         QPointF &pos,
                         int &dof)
    {
        pos = item->scenePos();
        if (const MassItem *m = dynamic_cast<const MassItem *>(item))
        {
            dof = sys.dofIndex.value(m, -1);
            return (dof >= 0);
        }
        dof = -1;
        return false;
    }

    static void assembleConnector(const SystemMatrices &sys,
                                  const QGraphicsItem *itemA,
                                  const QGraphicsItem *itemB,
                                  double val,
                                  QVector<double> &mat)
    {
        QPointF posA, posB;
        int dofA, dofB;
        bool freeA = nodeInfo(itemA, sys, posA, dofA);
        bool freeB = nodeInfo(itemB, sys, posB, dofB);

        if (!freeA && !freeB)
            return;

        QPointF delta = posB - posA;
        double len = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
        if (len < 1e-6)
            return;

        // Calculate direction cosine for X-axis
        double nx = delta.x() / len;

        // Effective 1D stiffness/damping projected onto X-axis
        double effectiveVal = val;

        auto addBlock = [&](int r, int c, double sign)
        {
            int n = sys.n;
            mat[r * n + c] += sign * effectiveVal;
        };

        if (freeA)
            addBlock(dofA, dofA, +1.0);
        if (freeB)
            addBlock(dofB, dofB, +1.0);
        if (freeA && freeB)
        {
            addBlock(dofA, dofB, -1.0);
            addBlock(dofB, dofA, -1.0);
        }
    }
};

#endif // SYSTEMBUILDER_H