#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QGraphicsView>
#include "simulationscene.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddMassClicked();
    void onAddSpringClicked();
    void onSpringAdded();
    void onAddDamperClicked();
    void onAddWallVClicked();
    void onAddWallHClicked();
    void onAddGroundClicked();
    void onAddForceClicked();
    void onExportClicked();
    void onFrfClicked();
    void onSensitivityClicked(); // ← NEW

private:
    QPushButton *addMassButton;
    QPushButton *addSpringButton;
    QPushButton *addDamperButton;
    QPushButton *addWallVButton;
    QPushButton *addWallHButton;
    QPushButton *addGroundButton;
    QPushButton *addForceButton;
    QPushButton *exportButton;
    QPushButton *frfButton;
    QPushButton *sensitivityButton; // ← NEW
    QGraphicsView *view;
    SimulationScene *scene;

    // ── helpers used by onSensitivityClicked ────────────────────────────────
    struct SensParam
    {
        QString name;          // e.g. "K[0][1]"
        QString type;          // "K", "C", or "M"
        double mean;           // mean Frobenius norm
        QVector<double> curve; // per-frequency sensitivity (length = N_freq)
    };

    // Parse sensitivity_output.csv  →  summary list
    QVector<SensParam> parseSensitivitySummary(const QString &summaryPath);

    // Parse sensitivity_vs_freq.csv  →  fills .curve on each param
    void parseSensitivityFreqCurves(const QString &freqPath,
                                    QVector<SensParam> &params,
                                    QVector<double> &omegas);

    // Open the three-tab results dialog
    void openSensitivityDialog(const QVector<SensParam> &params,
                               const QVector<double> &omegas);

    void exportPhysicalParameters(const QString &fileName);
};

#endif