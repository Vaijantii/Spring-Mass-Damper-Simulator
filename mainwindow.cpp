#include "simulationscene.h"
#include "mainwindow.h"
#include "massitem.h"
#include "wallitem.h"
#include "systembuilder.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QCoreApplication>
#include <QProcess>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>
#include <QTabWidget>
#include <QScrollArea>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    addToolBar(toolbar);

    // ── Items label ───────────────────────────────────────────────────────
    QLabel *itemLabel = new QLabel("Items: ", this);
    itemLabel->setStyleSheet("color: #cdd6f4; font-size: 9pt;");
    toolbar->addWidget(itemLabel);

    // ── Mass ──────────────────────────────────────────────────────────────
    addMassButton = new QPushButton("⬛ Mass", this);
    addMassButton->setFixedSize(80, 30);
    toolbar->addWidget(addMassButton);

    // ── Spring ────────────────────────────────────────────────────────────
    addSpringButton = new QPushButton("〰 Spring", this);
    addSpringButton->setFixedSize(90, 30);
    addSpringButton->setCheckable(true);
    toolbar->addWidget(addSpringButton);

    // ── Damper ────────────────────────────────────────────────────────────
    addDamperButton = new QPushButton("━━ Damper", this);
    addDamperButton->setFixedSize(100, 30);
    addDamperButton->setCheckable(true);
    toolbar->addWidget(addDamperButton);

    toolbar->addSeparator();

    // ── Force ─────────────────────────────────────────────────────────────
    addForceButton = new QPushButton("➡ Force", this);
    addForceButton->setFixedSize(85, 30);
    addForceButton->setCheckable(true);
    toolbar->addWidget(addForceButton);
    toolbar->addSeparator(); // Optional: visual separation

    // ── Walls (three variants) ────────────────────────────────────────────
    QLabel *wallLabel = new QLabel("  Walls: ", this);
    wallLabel->setStyleSheet("color: #cdd6f4; font-size: 9pt;");
    toolbar->addWidget(wallLabel);

    addWallVButton = new QPushButton("▐ Vertical", this);
    addWallVButton->setFixedSize(90, 30);
    addWallVButton->setToolTip("Add a vertical wall");
    toolbar->addWidget(addWallVButton);

    addWallHButton = new QPushButton("▬ Horizontal", this);
    addWallHButton->setFixedSize(105, 30);
    addWallHButton->setToolTip("Add a horizontal wall");
    toolbar->addWidget(addWallHButton);

    addGroundButton = new QPushButton("▓ Ground", this);
    addGroundButton->setFixedSize(90, 30);
    addGroundButton->setToolTip("Add a ground (fixed floor)");
    toolbar->addWidget(addGroundButton);

    toolbar->addSeparator();

    // ── Analysis ───────────────────────────────────────────────────────
    QLabel *analysisLabel = new QLabel("Analysis: ", this);
    analysisLabel->setStyleSheet("color: #cdd6f4; font-size: 9pt;");
    toolbar->addWidget(analysisLabel);

    // ── Export ────────────────────────────────────────────────────────────
    exportButton = new QPushButton("λ EVP", this);
    exportButton->setFixedSize(120, 30);
    toolbar->addWidget(exportButton);

    // ── FRF ───────────────────────────────────────────────────────────────
    frfButton = new QPushButton("📈 FRF", this);
    frfButton->setFixedSize(70, 30);
    frfButton->setToolTip("Frequency Response Function — plot |H_ij(ω)|");
    toolbar->addWidget(frfButton);

    // ── Sensitivity ───────────────────────────────────────────────────────
    sensitivityButton = new QPushButton("⚖ Sensitivity Analysis", this);
    sensitivityButton->setFixedSize(120, 30);
    sensitivityButton->setToolTip("Rank M, C, K parameters by influence on H(ω)");
    toolbar->addWidget(sensitivityButton);

    // ── Connections ───────────────────────────────────────────────────────
    connect(addMassButton, &QPushButton::clicked, this, &MainWindow::onAddMassClicked);
    connect(addSpringButton, &QPushButton::clicked, this, &MainWindow::onAddSpringClicked);
    connect(addDamperButton, &QPushButton::clicked, this, &MainWindow::onAddDamperClicked);
    connect(addWallVButton, &QPushButton::clicked, this, &MainWindow::onAddWallVClicked);
    connect(addWallHButton, &QPushButton::clicked, this, &MainWindow::onAddWallHClicked);
    connect(addGroundButton, &QPushButton::clicked, this, &MainWindow::onAddGroundClicked);
    connect(addForceButton, &QPushButton::clicked, this, &MainWindow::onAddForceClicked);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(frfButton, &QPushButton::clicked, this, &MainWindow::onFrfClicked);
    connect(sensitivityButton, &QPushButton::clicked,
            this, &MainWindow::onSensitivityClicked);

    // ── Scene ─────────────────────────────────────────────────────────────
    scene = new SimulationScene(this);
    scene->setSceneRect(0, 0, 800, 600);
    scene->setBackgroundBrush(QColor("#1e1e2e"));

    connect(scene, &SimulationScene::springAdded, this, &MainWindow::onSpringAdded);

    view = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setFrameStyle(QFrame::NoFrame);
    mainLayout->addWidget(view);

    setWindowTitle("Mass Spring Simulator");
    resize(1000, 650);
}

MainWindow::~MainWindow() {}

// ── helpers ──────────────────────────────────────────────────────────────────
static void uncheckAll(QPushButton *spring, QPushButton *damper, QPushButton *force)
{
    spring->setChecked(false);
    damper->setChecked(false);
    force->setChecked(false);
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void MainWindow::onAddMassClicked()
{
    uncheckAll(addSpringButton, addDamperButton, addForceButton);
    scene->setMode(InteractionMode::Select);

    bool ok;
    double mass = QInputDialog::getDouble(
        this, "Add Mass", "Enter mass (kg):",
        1.0, 0.001, 10000.0, 3, &ok);
    if (!ok)
        return;

    MassItem *m = new MassItem(mass);
    m->setIndex(scene->getMasses().size()); // index = current count before adding
    m->setPos(scene->sceneRect().center());
    scene->addItem(m);
}

void MainWindow::onAddSpringClicked()
{
    if (addSpringButton->isChecked())
    {
        addDamperButton->setChecked(false);
        addForceButton->setChecked(false);
        scene->setMode(InteractionMode::AddSpring);
    }
    else
    {
        scene->setMode(InteractionMode::Select);
    }
}

void MainWindow::onAddDamperClicked()
{
    if (addDamperButton->isChecked())
    {
        addSpringButton->setChecked(false);
        addForceButton->setChecked(false);
        scene->setMode(InteractionMode::AddDamper);
    }
    else
    {
        scene->setMode(InteractionMode::Select);
    }
}

void MainWindow::onAddWallVClicked()
{
    uncheckAll(addSpringButton, addDamperButton, addForceButton);
    scene->setMode(InteractionMode::Select);

    WallItem *wall = new WallItem(WallItem::WallType::Vertical, 120.0);
    wall->setPos(scene->sceneRect().center());
    scene->addItem(wall);
}

void MainWindow::onAddWallHClicked()
{
    uncheckAll(addSpringButton, addDamperButton, addForceButton);
    scene->setMode(InteractionMode::Select);

    WallItem *wall = new WallItem(WallItem::WallType::Horizontal, 120.0);
    wall->setPos(scene->sceneRect().center());
    scene->addItem(wall);
}

void MainWindow::onAddGroundClicked()
{
    uncheckAll(addSpringButton, addDamperButton, addForceButton);
    scene->setMode(InteractionMode::Select);

    // Ground defaults to scene width so it spans the full bottom area
    WallItem *ground = new WallItem(WallItem::WallType::Ground, 80.0);
    ground->setPos(scene->sceneRect().center());
    scene->addItem(ground);
}

void MainWindow::onAddForceClicked()
{
    if (addForceButton->isChecked())
    {
        addSpringButton->setChecked(false);
        addDamperButton->setChecked(false);
        scene->setMode(InteractionMode::AddForce);
    }
    else
    {
        scene->setMode(InteractionMode::Select);
    }
}

void MainWindow::onSpringAdded()
{
    uncheckAll(addSpringButton, addDamperButton, addForceButton);
}

QString readFrequenciesCSV(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return "Could not open natural_frequencies.csv";

    QTextStream in(&file);

    QString output;
    output += "Natural Frequencies\n\n";
    output += "Mode\tFrequency (rad/s)\n";
    output += "--------------------------------\n";

    bool firstLine = true;
    int modeNumber = 1;

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (firstLine)
        {
            firstLine = false;
            continue;
        }

        QStringList parts = line.split(",");
        if (parts.size() >= 2)
        {
            double freq = parts[1].toDouble();
            output += QString("%1\t%2\n")
                          .arg(modeNumber)
                          .arg(freq, 0, 'f', 4);
            modeNumber++;
        }
    }

    return output;
}

QString readModesCSV(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return "Could not open mode_shapes.csv";

    QTextStream in(&file);

    QString output;
    output += "\nModal Shapes\n\n";

    bool firstLine = true;

    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList parts = line.split(",");

        for (const QString &p : parts)
        {
            bool ok;
            double val = p.toDouble(&ok);

            if (ok)
                output += QString("%1\t").arg(val, 0, 'f', 4); // 4 decimals
            else
                output += QString("%1\t").arg(p); // keep "Mode0"
        }

        output += "\n";

        // Add separator AFTER header row
        if (firstLine)
        {
            output += "----------------------------------------\n";
            firstLine = false;
        }
    }

    return output;
}

void MainWindow::exportPhysicalParameters(const QString &fileName)
{
    QList<MassItem *> masses = scene->getMasses();
    QList<SpringItem *> springs = scene->getSprings();
    QList<DamperItem *> dampers = scene->getDampers();

    // Build the same DOF index map that SystemBuilder uses,
    // so spring/damper connectivity can be written as DOF indices.
    QMap<const MassItem *, int> dofIndex;
    for (int i = 0; i < masses.size(); ++i)
        dofIndex[masses[i]] = i;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    // ── Masses ──────────────────────────────────────────────────────────
    // Format:  m1 = 2.5   (one per line, DOF index = i)
    out << "Masses:\n";
    for (int i = 0; i < masses.size(); ++i)
        out << "m" << (i + 1) << " = " << masses[i]->getMass() << "\n";
    out << "\n";

    // ── Springs ─────────────────────────────────────────────────────────
    // Format:  k1 = 100   nodeA = 0   nodeB = 1
    //   nodeA / nodeB are DOF indices (-1 means wall/ground)
    out << "Springs:\n";
    for (int idx = 0; idx < springs.size(); ++idx)
    {
        SpringItem *s = springs[idx];

        int dofA = -1, dofB = -1;
        if (auto *m = dynamic_cast<MassItem *>(s->itemA()))
            dofA = dofIndex.value(m, -1);
        if (auto *m = dynamic_cast<MassItem *>(s->itemB()))
            dofB = dofIndex.value(m, -1);

        out << "k" << (idx + 1)
            << " = " << s->getStiffness()
            << "   nodeA = " << dofA
            << "   nodeB = " << dofB
            << "\n";
    }
    out << "\n";

    // ── Dampers ─────────────────────────────────────────────────────────
    // Format:  c1 = 10   nodeA = 0   nodeB = -1
    out << "Dampers:\n";
    for (int idx = 0; idx < dampers.size(); ++idx)
    {
        DamperItem *d = dampers[idx];

        int dofA = -1, dofB = -1;
        if (auto *m = dynamic_cast<MassItem *>(d->itemA()))
            dofA = dofIndex.value(m, -1);
        if (auto *m = dynamic_cast<MassItem *>(d->itemB()))
            dofB = dofIndex.value(m, -1);

        out << "c" << (idx + 1)
            << " = " << d->getDamping()
            << "   nodeA = " << dofA
            << "   nodeB = " << dofB
            << "\n";
    }
}

void MainWindow::onExportClicked()
{
    // 1. Gather all components from the scene
    QList<MassItem *> masses = scene->getMasses();
    QList<SpringItem *> springs = scene->getSprings();
    QList<DamperItem *> dampers = scene->getDampers();

    // 2. Build the system
    SystemMatrices sys = SystemBuilder::build(masses, springs, dampers);

    if (!sys.isValid())
    {
        QMessageBox::warning(this, "Export Failed", "The system has no valid degrees of freedom (you need to add at least one mass).");
        return;
    }

    // 3. Ask user where to save
    QString fileName = "matrix.txt";
    exportPhysicalParameters("physical_params.txt");

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);

    // Helper lambda to write a flat QVector as a 2D grid
    auto writeMatrix = [&](const QString &name, const QVector<double> &mat, int n)
    {
        out << name << " Matrix (" << n << "x" << n << "):\n";
        for (int r = 0; r < n; ++r)
        {
            for (int c = 0; c < n; ++c)
            {
                out << mat[r * n + c] << (c == n - 1 ? "" : ", ");
            }
            out << "\n";
        }
        out << "\n";
    };

    out << "System Matrix Export\n";
    out << "Degrees of Freedom (n): " << sys.n << "\n\n";

    // Write M, K, C
    writeMatrix("Mass (M)", sys.M, sys.n);
    writeMatrix("Stiffness (K)", sys.K, sys.n);
    writeMatrix("Damping (C)", sys.C, sys.n);

    // Write F(t)
    // Since F is time-dependent, we output the string expressions for each DOF
    out << "Force Vector F(t) Expressions:\n";
    for (int i = 0; i < sys.n; ++i)
    {
        QString expr = "0.0"; // Default to 0 if no force applied

        // Check if there's a force applied to this specific DOF
        for (const auto &ft : sys.forceTerms)
        {
            if (ft.dof == i)
            {
                // Combine the scale (direction cosine) with the expression
                expr = QString("%1 * (%2)").arg(ft.scale, 0, 'g', 4).arg(ft.fn->expression());
                break;
            }
        }
        out << sys.dofLabels[i] << " (DOF " << i << "): " << expr << "\n";
    }

    file.close();
    QMessageBox::information(this, "Success", "Matrices exported successfully to:\n" + fileName);

    // Get directory where user saved matrix file
    QString dir = QFileInfo(fileName).absolutePath();

    // -------- RUN QR SOLVER --------
    QString solverPath = QCoreApplication::applicationDirPath() + "/qr_solver.exe";

    QProcess *process = new QProcess(this);

    // Set working directory = where matrix file is saved
    process->setWorkingDirectory(QFileInfo(fileName).absolutePath());

    process->start(solverPath, QStringList() << fileName);

    if (!process->waitForStarted())
    {
        QMessageBox::critical(this, "Error",
                              "Could not start qr_solver.exe.\nCheck build folder.");
        return;
    }

    // When solver finishes
    connect(process, &QProcess::finished, this, [=](int exitCode, QProcess::ExitStatus status)
            {
            QString dir = QFileInfo(fileName).absolutePath();

            QString freqPath = dir + "/natural_frequencies.csv";
            QString modePath = dir + "/mode_shapes.csv";

            // Check if files created
            if (!QFile::exists(freqPath) || !QFile::exists(modePath))
            {
                QMessageBox::warning(this, "Error",
                                    "QR solver ran, but CSV files were not generated.");
                return;
            }

            QString freqText = readFrequenciesCSV(freqPath);
            QString modeText = readModesCSV(modePath);

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Modal Analysis Results");
            msgBox.setText(freqText + "\n" + modeText);

            // Use monospaced font for alignment
            QFont font("Courier New");
            msgBox.setFont(font);

            msgBox.exec(); });
}
// FRF
void MainWindow::onFrfClicked()
{
    QString workDir = QDir::currentPath();
    QString csvPath = workDir + "/H_output.csv";
    QString matPath = workDir + "/matrix.txt";

    // Helper: parse CSV and open plot dialog
    auto openFrfDialog = [this, csvPath]()
    {
        // 1. Read header to discover available DOFs
        QFile csvFile(csvPath);
        if (!csvFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, "FRF Error", "Cannot open H_output.csv.");
            return;
        }
        QTextStream csvIn(&csvFile);
        QString header = csvIn.readLine();
        csvFile.close();

        // Header: omega,H00,H01,H10,H11,...
        QStringList cols = header.split(',');
        int n = (int)std::round(std::sqrt((double)(cols.size() - 1)));

        QStringList dofLabels;
        for (int i = 0; i < n; ++i)
            dofLabels << QString("DOF %1 (Mass %2)").arg(i).arg(i + 1);

        // 2. DOF picker dialog
        QDialog picker(this);
        picker.setWindowTitle("Select DOF Pair for FRF");
        picker.setMinimumWidth(320);

        QVBoxLayout *vlay = new QVBoxLayout(&picker);
        vlay->addWidget(new QLabel("Input DOF  (force location — j):"));
        QComboBox *cbInput = new QComboBox(&picker);
        cbInput->addItems(dofLabels);
        vlay->addWidget(cbInput);

        vlay->addWidget(new QLabel("Output DOF  (response location — i):"));
        QComboBox *cbOutput = new QComboBox(&picker);
        cbOutput->addItems(dofLabels);
        vlay->addWidget(cbOutput);

        QDialogButtonBox *btns = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &picker);
        connect(btns, &QDialogButtonBox::accepted, &picker, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, &picker, &QDialog::reject);
        vlay->addWidget(btns);

        if (picker.exec() != QDialog::Accepted)
            return;

        int inputDof = cbInput->currentIndex();   // j
        int outputDof = cbOutput->currentIndex(); // i

        // Column in CSV: 1 + outputDof*n + inputDof  (row-major H[i][j])
        int colIndex = 1 + outputDof * n + inputDof;

        // 3. Read natural_frequencies.csv
        QString freqCsvPath = QDir::currentPath() + "/natural_frequencies.csv";
        QVector<double> natFreqs_hz;
        QVector<double> natFreqs_rad;
        {
            QFile fFile(freqCsvPath);
            if (fFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream fIn(&fFile);
                fIn.readLine();
                while (!fIn.atEnd())
                {
                    QString line = fIn.readLine().trimmed();
                    if (line.isEmpty())
                        continue;
                    QStringList parts = line.split(',');
                    if (parts.size() >= 2)
                    {
                        double rad = parts[1].toDouble();
                        natFreqs_rad.append(rad);
                        natFreqs_hz.append(rad / (2.0 * M_PI));
                    }
                }
            }
        }

        // 4. Read M, C, K diagonals from matrix.txt for ζ = C_ii / (2√(K_ii·M_ii))
        QString matTxtPath = QDir::currentPath() + "/matrix.txt";
        QVector<double> diagM, diagK, diagC;
        {
            QFile mf(matTxtPath);
            if (mf.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream ms(&mf);
                QString section;
                int rowIdx = 0;
                while (!ms.atEnd())
                {
                    QString line = ms.readLine().trimmed();
                    if (line.startsWith("Mass (M)"))
                    {
                        section = "M";
                        rowIdx = 0;
                        continue;
                    }
                    if (line.startsWith("Stiffness (K)"))
                    {
                        section = "K";
                        rowIdx = 0;
                        continue;
                    }
                    if (line.startsWith("Damping (C)"))
                    {
                        section = "C";
                        rowIdx = 0;
                        continue;
                    }
                    if (line.startsWith("Force") || line.startsWith("System"))
                    {
                        section = "";
                        continue;
                    }
                    if (section.isEmpty() || line.isEmpty())
                        continue;

                    // Parse the row, grab the diagonal element
                    QString cleaned = line;
                    cleaned.replace(',', ' ');
                    QTextStream rs(&cleaned);
                    double val = 0.0;
                    for (int c = 0; c <= rowIdx; ++c)
                        rs >> val;

                    if (section == "M")
                        diagM.append(val);
                    else if (section == "K")
                        diagK.append(val);
                    else if (section == "C")
                        diagC.append(val);
                    ++rowIdx;
                }
            }
        }

        // // Compute ζ_i per DOF from diagonal (modal damping approximation)
        // QVector<double> zetaDiag;
        // int nDiag = std::min({diagM.size(), diagK.size(), diagC.size()});
        // for (int i = 0; i < nDiag; ++i)
        // {
        //     double denom = 2.0 * std::sqrt(diagK[i] * diagM[i]);
        //     zetaDiag.append(denom > 1e-12 ? diagC[i] / denom : 0.0);
        // }

        // 5. Read H_output.csv — store all (omega_hz, mag) points + track peaks
        QFile dataFile(csvPath);
        if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, "FRF Error", "Cannot reopen H_output.csv.");
            return;
        }
        QTextStream dataIn(&dataFile);
        dataIn.readLine();

        struct DataPoint
        {
            double freq_hz;
            double mag;
        };
        QVector<DataPoint> allPoints;
        double maxMag = 0.0;

        struct ModeResult
        {
            double peakMag;
            double closestDist;
        };
        QVector<ModeResult> modeResults(natFreqs_rad.size(), {0.0, 1e18});

        while (!dataIn.atEnd())
        {
            QString line = dataIn.readLine().trimmed();
            if (line.isEmpty())
                continue;
            QStringList parts = line.split(',');
            if (parts.size() <= colIndex)
                continue;

            double omega = parts[0].toDouble();
            double mag = parts[colIndex].toDouble();
            double freq_hz = omega / (2.0 * M_PI);
            allPoints.append({freq_hz, mag});
            if (mag > maxMag)
                maxMag = mag;

            for (int m = 0; m < natFreqs_rad.size(); ++m)
            {
                double dist = std::abs(omega - natFreqs_rad[m]);
                if (dist < modeResults[m].closestDist)
                {
                    modeResults[m].closestDist = dist;
                    // Only accept if reasonably close to resonance
                    if (dist < 0.1 * natFreqs_rad[m]) // <-- ADD THIS CONDITION
                    {
                        modeResults[m].peakMag = mag;
                    }
                }
            }
        }
        dataFile.close();

        // 6. Half-power bandwidth per mode
        // For each mode find f_lo and f_hi where |H| first crosses peak/√2
        // on each side of the peak frequency. ζ_hp ≈ Δf / (2·fₙ)
        struct HalfPower
        {
            double f_lo;
            double f_hi;
            double bw;
            double zeta_hp;
            bool valid;
        };
        QVector<HalfPower> halfPowers(natFreqs_rad.size(), {0, 0, 0, 0, false});

        for (int m = 0; m < natFreqs_rad.size(); ++m)
        {
            double fn_hz = natFreqs_hz[m];
            double halfVal = modeResults[m].peakMag / std::sqrt(2.0);

            double f_lo = -1.0, f_hi = -1.0;

            // Scan left of peak for lower half-power crossing
            for (int k = 1; k < allPoints.size(); ++k)
            {
                if (allPoints[k].freq_hz >= fn_hz)
                    break;

                if ((allPoints[k - 1].mag - halfVal) * (allPoints[k].mag - halfVal) <= 0.0)
                {
                    f_lo = allPoints[k].freq_hz;
                }
            }
            // Scan right of peak for upper half-power crossing
            for (int k = 1; k < allPoints.size(); ++k)
            {
                if (allPoints[k].freq_hz <= fn_hz)
                    continue;

                if ((allPoints[k - 1].mag - halfVal) * (allPoints[k].mag - halfVal) <= 0.0)
                {
                    f_hi = allPoints[k].freq_hz;
                    break;
                }
            }

            if (f_lo > 0.0 && f_hi > 0.0 && fn_hz > 1e-9)
            {
                double bw = f_hi - f_lo;
                double zeta_hp = bw / (2.0 * fn_hz);
                halfPowers[m] = {f_lo, f_hi, bw, zeta_hp, true};
            }
        }

        // 7. Build main FRF series
        QLineSeries *series = new QLineSeries();
        series->setColor(QColor("#89b4fa"));
        QPen seriesPen(QColor("#89b4fa"));
        seriesPen.setWidth(2);
        series->setPen(seriesPen);
        for (const auto &pt : allPoints)
            series->append(pt.freq_hz, pt.mag);

        // 8. Build chart
        QChart *chart = new QChart();
        chart->addSeries(series);
        chart->setTitle(QString("|H_%1%2(ω)|  —  Output DOF %1, Input DOF %2")
                            .arg(outputDof)
                            .arg(inputDof));
        // chart->setTheme(QChart::ChartThemeDark);
        chart->legend()->hide();

        QValueAxis *axisX = new QValueAxis();
        axisX->setTitleText("Frequency (Hz)");
        axisX->setLabelFormat("%.2f");
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        axisY->setTitleText("|H| (m/N)");
        axisY->setLabelFormat("%.4f");
        axisY->setRange(0.0, maxMag * 1.15);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);

        QScatterSeries *peaks = new QScatterSeries();
        peaks->setMarkerSize(10);
        peaks->setColor(QColor("#f9e2af")); // amber

        for (int m = 0; m < natFreqs_rad.size(); ++m)
        {
            double fn_hz = natFreqs_hz[m];

            double bestDist = 1e9;
            double bestMag = 0.0;

            for (const auto &pt : allPoints)
            {
                double d = std::abs(pt.freq_hz - fn_hz);
                if (d < bestDist)
                {
                    bestDist = d;
                    bestMag = pt.mag;
                }
            }

            if (bestDist < 0.02) // tolerance
                peaks->append(fn_hz, bestMag);
        }

        chart->addSeries(peaks);
        peaks->attachAxis(axisX);
        peaks->attachAxis(axisY);

        for (int m = 0; m < natFreqs_rad.size(); ++m)
        {
            double fn_hz = natFreqs_hz[m];

            QLineSeries *vline = new QLineSeries();
            QPen pen(QColor("#a6e3a1")); // green
            pen.setStyle(Qt::DashLine);
            vline->setPen(pen);

            vline->append(fn_hz, 0.0);
            vline->append(fn_hz, maxMag);

            chart->addSeries(vline);
            vline->attachAxis(axisX);
            vline->attachAxis(axisY);
        }

        QScatterSeries *hpPoints = new QScatterSeries();
        hpPoints->setMarkerSize(8);
        hpPoints->setColor(QColor("#f38ba8")); // red

        for (int m = 0; m < natFreqs_rad.size(); ++m)
        {
            if (!halfPowers[m].valid)
                continue;

            double halfVal = modeResults[m].peakMag / std::sqrt(2.0);

            hpPoints->append(halfPowers[m].f_lo, halfVal);
            hpPoints->append(halfPowers[m].f_hi, halfVal);
        }

        chart->addSeries(hpPoints);
        hpPoints->attachAxis(axisX);
        hpPoints->attachAxis(axisY);

        // 9. Overlay half-power horizontal lines per mode
        double xMin = allPoints.isEmpty() ? 0.0 : allPoints.first().freq_hz;
        double xMax = allPoints.isEmpty() ? 1.0 : allPoints.last().freq_hz;

        for (int m = 0; m < natFreqs_rad.size(); ++m)
        {
            if (natFreqs_rad[m] < 1e-6)
                continue;
            if (!halfPowers[m].valid || natFreqs_rad[m] < 1e-6)
                continue;
            double halfVal = modeResults[m].peakMag / std::sqrt(2.0);

            QLineSeries *hpLine = new QLineSeries();
            QPen hpPen(QColor("#f38ba8")); // red
            hpPen.setWidth(1);
            hpPen.setStyle(Qt::DashLine);
            hpLine->setPen(hpPen);
            hpLine->append(xMin, halfVal);
            hpLine->append(xMax, halfVal);
            chart->addSeries(hpLine);
            hpLine->attachAxis(axisX);
            hpLine->attachAxis(axisY);
        }

        // 10. Results table — 7 columns
        QTableWidget *table = new QTableWidget();
        table->setColumnCount(7);
        table->setHorizontalHeaderLabels({"Mode", "ωₙ (rad/s)", "fₙ (Hz)",
                                          "|H_ij| at ωₙ", "ζ (modal)", "Δf (Hz)", "ζ (½-pwr)"});
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setMinimumWidth(480);
        table->setRowCount(natFreqs_rad.size());
        table->setStyleSheet(
            "QTableWidget { background: #1e1e2e; color: #cdd6f4; "
            "  gridline-color: #313244; font-size: 11px; }"
            "QHeaderView::section { background: #313244; color: #cdd6f4; "
            "  padding: 4px; border: none; font-weight: bold; }"
            "QTableWidget::item { padding: 4px; }");

        for (int m = 0; m < natFreqs_rad.size(); ++m)
        {
            // ζ from diagonal (use same DOF index as mode, capped to available)
            double zetaVal = (halfPowers[m].valid) ? halfPowers[m].zeta_hp : -1.0;
            QString zetaStr = (zetaVal >= 0.0)
                                  ? QString::number(zetaVal, 'f', 4)
                                  : "—";
            QString dampStatus;
            QColor zetaColor = QColor("#cdd6f4");
            if (zetaVal >= 0.0)
            {
                if (zetaVal < 1.0)
                {
                    dampStatus = " (underdamped)";
                    zetaColor = QColor("#a6e3a1");
                }
                else if (zetaVal > 1.0)
                {
                    dampStatus = " (overdamped)";
                    zetaColor = QColor("#f38ba8");
                }
                else
                {
                    dampStatus = " (critical)";
                    zetaColor = QColor("#f9e2af");
                }
                zetaStr += dampStatus;
            }

            QString bwStr, zetaHpStr;
            if (halfPowers[m].valid)
            {
                bwStr = QString::number(halfPowers[m].bw, 'f', 4);
                zetaHpStr = QString::number(halfPowers[m].zeta_hp, 'f', 4);
            }
            else
            {
                bwStr = "—";
                zetaHpStr = "—";
            }

            auto makeItem = [](const QString &txt)
            {
                auto *it = new QTableWidgetItem(txt);
                it->setTextAlignment(Qt::AlignCenter);
                return it;
            };

            auto *modeItem = makeItem(QString("Mode %1").arg(m + 1));
            auto *radItem = makeItem(QString::number(natFreqs_rad[m], 'f', 4));
            auto *hzItem = makeItem(QString::number(natFreqs_hz[m], 'f', 4));
            auto *peakItem = makeItem(QString::number(modeResults[m].peakMag, 'g', 5));
            auto *zetaItem = makeItem(zetaStr);
            auto *bwItem = makeItem(bwStr);
            auto *zetaHpItem = makeItem(zetaHpStr);

            zetaItem->setForeground(zetaColor);

            // Amber highlight for dominant mode
            if (modeResults[m].peakMag == maxMag)
                for (auto *it : {modeItem, radItem, hzItem, peakItem, bwItem, zetaHpItem})
                    it->setForeground(QColor("#f9e2af"));

            table->setItem(m, 0, modeItem);
            table->setItem(m, 1, radItem);
            table->setItem(m, 2, hzItem);
            table->setItem(m, 3, peakItem);
            table->setItem(m, 4, zetaItem);
            table->setItem(m, 5, bwItem);
            table->setItem(m, 6, zetaHpItem);
        }

        // 11. Plot window
        QDialog *plotDlg = new QDialog(this);
        plotDlg->setWindowTitle(QString("FRF Plot — H_%1%2")
                                    .arg(outputDof)
                                    .arg(inputDof));
        plotDlg->resize(1300, 540);
        plotDlg->setAttribute(Qt::WA_DeleteOnClose);

        QVBoxLayout *outerLay = new QVBoxLayout(plotDlg);
        outerLay->setContentsMargins(8, 8, 8, 8);
        outerLay->setSpacing(6);

        // Chart + table side by side
        QHBoxLayout *topRow = new QHBoxLayout();
        topRow->setSpacing(8);

        QChartView *chartView = new QChartView(chart, plotDlg);
        chartView->setRenderHint(QPainter::Antialiasing);
        topRow->addWidget(chartView, 1);

        QVBoxLayout *tablePanel = new QVBoxLayout();
        QLabel *tableTitle = new QLabel("Modal Summary");
        tableTitle->setStyleSheet("color: #cdd6f4; font-weight: bold; "
                                  "font-size: 12px; padding: 4px 0;");
        tableTitle->setAlignment(Qt::AlignCenter);
        tablePanel->addWidget(tableTitle);
        tablePanel->addWidget(table);
        topRow->addLayout(tablePanel);
        outerLay->addLayout(topRow);

        // Legend row at bottom
        QHBoxLayout *legend = new QHBoxLayout();
        auto makeLegendLabel = [](const QString &color, const QString &text)
        {
            QLabel *l = new QLabel(
                QString("<span style='color:%1'>■</span> %2").arg(color, text));
            l->setStyleSheet("font-size: 10px; color: #cdd6f4;");
            return l;
        };
        legend->addStretch();
        legend->addWidget(makeLegendLabel("#89b4fa", "|H_ij(ω)|"));
        legend->addSpacing(16);
        legend->addWidget(makeLegendLabel("#f38ba8", "half-power level (peak/√2)"));
        legend->addSpacing(16);
        legend->addWidget(makeLegendLabel("#f38ba8", "half-power points"));
        legend->addSpacing(16);
        legend->addWidget(makeLegendLabel("#f9e2af", "peaks"));
        legend->addSpacing(16);
        legend->addWidget(makeLegendLabel("#a6e3a1", "natural frequency"));
        legend->addStretch();
        outerLay->addLayout(legend);

        plotDlg->show();
    };

    // If CSV already exists, skip solver
    if (QFile::exists(csvPath))
    {
        QFile::remove(csvPath);
    }

    // First run: need matrix.txt present
    if (!QFile::exists(matPath))
    {
        QMessageBox::warning(this, "FRF Error",
                             "matrix.txt not found in build directory.\n"
                             "Please click 'Export Matrices' first.");
        return;
    }

    QString luPath = QCoreApplication::applicationDirPath() + "/lu_solver.exe";
    if (!QFile::exists(luPath))
    {
        QMessageBox::critical(this, "FRF Error",
                              "lu_solver.exe not found.\n"
                              "Add to CMakeLists.txt:\n\n"
                              "  add_executable(lu_solver lu.cpp)\n"
                              "  set_target_properties(lu_solver PROPERTIES\n"
                              "    RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}\")");
        return;
    }

    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(workDir);
    proc->start(luPath);

    if (!proc->waitForStarted(3000))
    {
        QMessageBox::critical(this, "FRF Error", "Could not start lu_solver.exe.");
        delete proc;
        return;
    }

    frfButton->setEnabled(false);
    frfButton->setText("⏳ Computing…");

    connect(proc, &QProcess::finished, this,
            [this, proc, csvPath, openFrfDialog](int, QProcess::ExitStatus)
            {
                frfButton->setEnabled(true);
                frfButton->setText("📈 FRF");
                proc->deleteLater();

                if (!QFile::exists(csvPath))
                {
                    QMessageBox::warning(this, "FRF Error",
                                         "lu_solver ran but H_output.csv was not created.\n"
                                         "Check matrix.txt is in the build folder.");
                    return;
                }
                openFrfDialog();
            });
}

// ============================================================
// SENSITIVITY ANALYSIS  —  append this block to mainwindow.cpp
// ============================================================

QVector<MainWindow::SensParam>
MainWindow::parseSensitivitySummary(const QString &summaryPath)
{
    QVector<SensParam> params;
    QFile f(summaryPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return params;

    QTextStream in(&f);
    in.readLine(); // skip header: Rank,Parameter,MeanSensitivity

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QStringList p = line.split(',');
        if (p.size() < 3)
            continue;

        SensParam sp;
        sp.name = p[1]; // e.g. "K[0][1]"
        sp.mean = p[2].toDouble();
        sp.type = sp.name.left(1).toUpper(); // first char: K, C, or M
        params.append(sp);
    }
    return params;
}

// parseSensitivityFreqCurves
void MainWindow::parseSensitivityFreqCurves(const QString &freqPath,
                                            QVector<SensParam> &params,
                                            QVector<double> &omegas)
{
    QFile f(freqPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&f);
    QString header = in.readLine();
    QStringList cols = header.split(',');
    // cols[0] = "omega", cols[1..] = param names in ranked order

    // Build name→index map into params
    QMap<QString, int> nameIdx;
    for (int i = 0; i < params.size(); ++i)
        nameIdx[params[i].name] = i;

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QStringList vals = line.split(',');
        if (vals.size() < 2)
            continue;

        omegas.append(vals[0].toDouble());
        for (int c = 1; c < qMin(vals.size(), cols.size()); ++c)
        {
            QString pname = cols[c];
            if (nameIdx.contains(pname))
                params[nameIdx[pname]].curve.append(vals[c].toDouble());
        }
    }
}

// openSensitivityDialog
void MainWindow::openSensitivityDialog(const QVector<SensParam> &params,
                                       const QVector<double> &omegas)
{
    if (params.isEmpty())
    {
        QMessageBox::warning(this, "Sensitivity", "No data to display.");
        return;
    }

    // colour helpers
    // Sensitivity tier thresholds (relative to max)
    double maxMean = params.first().mean; // already sorted descending
    auto tierColor = [&](double v) -> QColor
    {
        double ratio = v / maxMean;
        if (ratio >= 0.6)
            return QColor("#f38ba8"); // red   – critical
        if (ratio >= 0.3)
            return QColor("#f9e2af"); // amber – medium
        return QColor("#a6e3a1");     // green – low
    };
    auto typeColor = [](const QString &t) -> QColor
    {
        if (t == "K")
            return QColor("#89b4fa"); // blue
        if (t == "C")
            return QColor("#a6e3a1"); // green
        return QColor("#f9e2af");     // amber – M
    };

    //  master dialog
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("Sensitivity Analysis Results");
    dlg->resize(1050, 620);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet(
        "QDialog { background:#1e1e2e; color:#cdd6f4; }"
        "QTabWidget::pane { border:1px solid #313244; background:#1e1e2e; }"
        "QTabBar::tab { background:#313244; color:#cdd6f4; padding:6px 18px;"
        "  border-radius:4px; margin-right:2px; font-size:11px; }"
        "QTabBar::tab:selected { background:#45475a; color:#cdd6f4; }"
        "QLabel { color:#cdd6f4; }"
        "QScrollArea { border:none; background:#1e1e2e; }");

    QVBoxLayout *mainLay = new QVBoxLayout(dlg);
    mainLay->setContentsMargins(10, 10, 10, 10);

    QTabWidget *tabs = new QTabWidget(dlg);
    mainLay->addWidget(tabs);

    // ------------------------------------------------------------------------------
    // TAB 1 — BAR CHART  (horizontal, styled like the screenshot)
    // ------------------------------------------------------------------------------
    {
        QWidget *tab1 = new QWidget();
        QVBoxLayout *lay = new QVBoxLayout(tab1);
        lay->setContentsMargins(12, 12, 12, 12);
        lay->setSpacing(6);

        // title + legend row
        QHBoxLayout *titleRow = new QHBoxLayout();
        QLabel *title = new QLabel("Parameter ranking  —  mean  ‖Sₚ‖F  over frequency");
        title->setStyleSheet("font-size:13px; font-weight:bold; color:#cdd6f4;");
        titleRow->addWidget(title);
        titleRow->addStretch();

        auto makeLeg = [](const QString &col, const QString &txt)
        {
            QLabel *l = new QLabel(
                QString("<span style='color:%1'>■</span> <span style='color:#9399b2'>%2</span>")
                    .arg(col, txt));
            l->setStyleSheet("font-size:10px;");
            return l;
        };
        titleRow->addWidget(makeLeg("#89b4fa", "K  stiffness"));
        titleRow->addSpacing(8);
        titleRow->addWidget(makeLeg("#a6e3a1", "C  damping"));
        titleRow->addSpacing(8);
        titleRow->addWidget(makeLeg("#f9e2af", "M  mass"));
        lay->addLayout(titleRow);

        // bar rows (custom painted widget, no Qt Charts needed)
        QScrollArea *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        QWidget *barContainer = new QWidget();
        barContainer->setStyleSheet("background:#1e1e2e;");
        QVBoxLayout *barLay = new QVBoxLayout(barContainer);
        barLay->setSpacing(5);
        barLay->setContentsMargins(0, 4, 0, 4);

        for (int i = 0; i < params.size(); ++i)
        {
            const SensParam &p = params[i];
            double ratio = p.mean / maxMean;

            QWidget *row = new QWidget();
            row->setFixedHeight(34);
            row->setStyleSheet("background:transparent;");
            QHBoxLayout *rl = new QHBoxLayout(row);
            rl->setContentsMargins(0, 0, 0, 0);
            rl->setSpacing(8);

            // Rank
            QLabel *rank = new QLabel(QString::number(i + 1));
            rank->setFixedWidth(18);
            rank->setStyleSheet("color:#585b70; font-size:11px;");
            rank->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rl->addWidget(rank);

            // Param name (monospace)
            QLabel *name = new QLabel(p.name);
            name->setFixedWidth(70);
            name->setStyleSheet("font-family:monospace; font-size:12px; color:#cdd6f4;");
            rl->addWidget(name);

            // Type badge
            QLabel *badge = new QLabel(p.type);
            badge->setFixedSize(22, 22);
            badge->setAlignment(Qt::AlignCenter);
            badge->setStyleSheet(QString(
                                     "background:%1; color:#1e1e2e; border-radius:11px;"
                                     " font-size:10px; font-weight:bold;")
                                     .arg(typeColor(p.type).name()));
            rl->addWidget(badge);

            // Bar track
            QWidget *track = new QWidget();
            track->setFixedHeight(20);
            track->setStyleSheet("background:#313244; border-radius:3px;");
            QHBoxLayout *tl = new QHBoxLayout(track);
            tl->setContentsMargins(0, 0, 0, 0);
            tl->setSpacing(0);

            QWidget *fill = new QWidget();
            fill->setStyleSheet(QString("background:%1; border-radius:3px;")
                                    .arg(typeColor(p.type).name()));
            tl->addWidget(fill);
            tl->addStretch(1000); // fills remaining space

            // Force fill width by stretch ratio
            // We use stretch: fill gets ratio*1000, spacer gets (1-ratio)*1000
            fill->setMaximumWidth(10000);
            // Re-do with proper stretch
            QHBoxLayout *tl2 = new QHBoxLayout(track);
            tl2->setContentsMargins(0, 0, 0, 0);
            tl2->setSpacing(0);

            QWidget *fillBar = new QWidget();
            fillBar->setStyleSheet(QString("background:%1; border-radius:3px;")
                                       .arg(typeColor(p.type).name()));
            tl2->addWidget(fillBar, qRound(ratio * 1000));
            QWidget *spacer = new QWidget();
            spacer->setStyleSheet("background:transparent;");
            tl2->addWidget(spacer, qRound((1.0 - ratio) * 1000));
            delete tl; // remove old layout
            track->setLayout(tl2);

            rl->addWidget(track, 1);

            // Value
            QLabel *val = new QLabel(QString::number(p.mean, 'f', 3));
            val->setFixedWidth(48);
            val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            val->setStyleSheet("font-family:monospace; font-size:12px; color:#9399b2;");
            rl->addWidget(val);

            // Tier dot (red/amber/green)
            QLabel *tier = new QLabel("●");
            tier->setFixedWidth(16);
            tier->setAlignment(Qt::AlignCenter);
            tier->setStyleSheet(QString("color:%1; font-size:14px;")
                                    .arg(tierColor(p.mean).name()));
            rl->addWidget(tier);

            barLay->addWidget(row);
        }
        barLay->addStretch();
        scroll->setWidget(barContainer);
        lay->addWidget(scroll, 1);

        // tier legend
        QHBoxLayout *tierLeg = new QHBoxLayout();
        tierLeg->addStretch();
        auto makeTier = [](const QString &col, const QString &txt)
        {
            QLabel *l = new QLabel(
                QString("<span style='color:%1'>●</span>"
                        " <span style='color:#9399b2'>%2</span>")
                    .arg(col, txt));
            l->setStyleSheet("font-size:10px;");
            return l;
        };
        tierLeg->addWidget(makeTier("#f38ba8", "critical  (≥60% of max)"));
        tierLeg->addSpacing(12);
        tierLeg->addWidget(makeTier("#f9e2af", "medium   (30–60%)"));
        tierLeg->addSpacing(12);
        tierLeg->addWidget(makeTier("#a6e3a1", "low   (<30%)"));
        tierLeg->addStretch();
        lay->addLayout(tierLeg);

        tabs->addTab(tab1, "📊  Bar chart");
    }

    // ------------------------------------------------------------------------------
    // TAB 2 — SCENE OVERLAY  (color-coded list + live scene glow)
    // ------------------------------------------------------------------------------
    {
        QWidget *tab2 = new QWidget();
        QHBoxLayout *lay = new QHBoxLayout(tab2);
        lay->setContentsMargins(12, 12, 12, 12);
        lay->setSpacing(16);

        // LEFT: colour key list
        QVBoxLayout *listLay = new QVBoxLayout();

        QLabel *lbl = new QLabel("Element sensitivity classification");
        lbl->setStyleSheet("font-size:12px; font-weight:bold; color:#cdd6f4; margin-bottom:6px;");
        listLay->addWidget(lbl);

        for (int i = 0; i < params.size(); ++i)
        {
            const SensParam &p = params[i];
            QColor tColor = tierColor(p.mean);

            QWidget *row = new QWidget();
            row->setFixedHeight(32);
            row->setStyleSheet(QString(
                                   "background:#181825; border-left:3px solid %1;"
                                   " border-radius:4px; padding:0 8px;")
                                   .arg(tColor.name()));
            QHBoxLayout *rl = new QHBoxLayout(row);
            rl->setContentsMargins(8, 0, 8, 0);

            QLabel *nm = new QLabel(p.name);
            nm->setStyleSheet("font-family:monospace; font-size:12px; color:#cdd6f4;");
            rl->addWidget(nm);
            rl->addStretch();

            QLabel *tv = new QLabel(QString::number(p.mean, 'f', 3));
            tv->setStyleSheet(QString("font-size:11px; color:%1;").arg(tColor.name()));
            rl->addWidget(tv);

            // Tier label
            QString tierLbl;
            double ratio = p.mean / maxMean;
            if (ratio >= 0.6)
                tierLbl = "  CRITICAL";
            else if (ratio >= 0.3)
                tierLbl = "  MEDIUM";
            else
                tierLbl = "  LOW";
            QLabel *tl = new QLabel(tierLbl);
            tl->setStyleSheet(QString("font-size:10px; color:%1;").arg(tColor.name()));
            tl->setFixedWidth(70);
            rl->addWidget(tl);

            listLay->addWidget(row);
        }
        listLay->addStretch();
        lay->addLayout(listLay, 1);

        // RIGHT: colour key explanation
        QVBoxLayout *keyLay = new QVBoxLayout();
        keyLay->setAlignment(Qt::AlignTop);

        QLabel *keyTitle = new QLabel("How to read the scene");
        keyTitle->setStyleSheet("font-size:12px; font-weight:bold; color:#cdd6f4;");
        keyLay->addWidget(keyTitle);
        keyLay->addSpacing(10);

        struct KeyEntry
        {
            QString color;
            QString text;
        };
        QVector<KeyEntry> keys = {
            {"#f38ba8", "🔴  Critical  — this element has the largest influence on H(ω). Measure it accurately."},
            {"#f9e2af", "🟠  Medium  — moderate effect. Worth checking but not the main source of error."},
            {"#a6e3a1", "🟢  Low  — small influence. Even a large measurement error barely shifts H(ω)."},
        };

        for (auto &k : keys)
        {
            QLabel *kl = new QLabel(k.text);
            kl->setStyleSheet(QString("color:%1; font-size:11px; "
                                      "background:#181825; border-radius:6px; "
                                      "padding:8px;")
                                  .arg(k.color));
            kl->setWordWrap(true);
            keyLay->addWidget(kl);
            keyLay->addSpacing(8);
        }

        keyLay->addStretch();
        lay->addLayout(keyLay);

        tabs->addTab(tab2, "🎨  Scene overlay");

        scene->update(); // repaint scene with new colors
    }

    // ------------------------------------------------------------------------------
    // TAB 3 — FREQUENCY-DEPENDENT PLOT  (top 5 params, line chart)
    // ------------------------------------------------------------------------------
    {
        QWidget *tab3 = new QWidget();
        QVBoxLayout *lay = new QVBoxLayout(tab3);
        lay->setContentsMargins(12, 12, 12, 12);
        lay->setSpacing(8);

        if (omegas.isEmpty() || params.first().curve.isEmpty())
        {
            lay->addWidget(new QLabel(
                "sensitivity_vs_freq.csv not found.\n"
                "Run the sensitivity solver first."));
        }
        else
        {
            // Line chart
            QChart *chart = new QChart();
            chart->setBackgroundBrush(QColor("#1e1e2e"));
            chart->setPlotAreaBackgroundBrush(QColor("#181825"));
            chart->setPlotAreaBackgroundVisible(true);
            chart->setMargins(QMargins(4, 4, 4, 4));
            chart->legend()->setVisible(false); // custom legend below

            // X-axis in rad/s
            QValueAxis *axX = new QValueAxis();
            axX->setTitleText("ω  (rad/s)");
            axX->setLabelFormat("%.2f");
            axX->setTitleBrush(QColor("#9399b2"));
            axX->setLabelsColor(QColor("#9399b2"));
            axX->setGridLineColor(QColor("#313244"));
            axX->setLinePen(QPen(QColor("#45475a")));
            chart->addAxis(axX, Qt::AlignBottom);

            QValueAxis *axY = new QValueAxis();
            axY->setTitleText("‖Sₚ(ω)‖F");
            axY->setLabelFormat("%.2f");
            axY->setTitleBrush(QColor("#9399b2"));
            axY->setLabelsColor(QColor("#9399b2"));
            axY->setGridLineColor(QColor("#313244"));
            axY->setLinePen(QPen(QColor("#45475a")));
            chart->addAxis(axY, Qt::AlignLeft);

            // Colour palette for top-5 lines
            QVector<QColor> palette = {
                QColor("#f38ba8"), // red
                QColor("#89b4fa"), // blue
                QColor("#a6e3a1"), // green
                QColor("#f9e2af"), // yellow
                QColor("#cba6f7"), // purple
            };

            int topN = qMin(5, params.size());
            double yMax = 0.0;
            double xMin = omegas.isEmpty() ? 0 : omegas.first();
            double xMax = omegas.isEmpty() ? 1 : omegas.last();

            for (int pi = 0; pi < topN; ++pi)
            {
                const SensParam &p = params[pi];
                if (p.curve.isEmpty())
                    continue;

                QLineSeries *series = new QLineSeries();
                QPen pen(palette[pi % palette.size()]);
                pen.setWidthF(2.0);
                series->setPen(pen);

                int nPts = qMin(omegas.size(), p.curve.size());
                for (int k = 0; k < nPts; ++k)
                {
                    series->append(omegas[k], p.curve[k]);
                    if (p.curve[k] > yMax)
                        yMax = p.curve[k];
                }
                chart->addSeries(series);
                series->attachAxis(axX);
                series->attachAxis(axY);
            }

            axX->setRange(xMin, xMax);
            axY->setRange(0, yMax * 1.1);

            QChartView *cv = new QChartView(chart, tab3);
            cv->setRenderHint(QPainter::Antialiasing);
            lay->addWidget(cv, 1);

            // Custom legend
            QHBoxLayout *legRow = new QHBoxLayout();
            legRow->addStretch();
            for (int pi = 0; pi < topN; ++pi)
            {
                QLabel *dot = new QLabel("━━");
                dot->setStyleSheet(QString("color:%1; font-size:14px;")
                                       .arg(palette[pi % palette.size()].name()));
                legRow->addWidget(dot);
                QLabel *nm = new QLabel(params[pi].name);
                nm->setStyleSheet("font-family:monospace; font-size:11px; color:#9399b2;");
                legRow->addWidget(nm);
                legRow->addSpacing(12);
            }
            legRow->addStretch();
            lay->addLayout(legRow);

            // Explanation note
            QLabel *note = new QLabel(
                "Each curve shows ‖Sₚ(ω)‖F — how much H(ω) reacts to a 1% change in "
                "parameter p at each frequency. Peaks near natural frequencies (ω₁, ω₂) "
                "indicate resonance-driven sensitivity.");
            note->setStyleSheet("font-size:10px; color:#585b70;");
            note->setWordWrap(true);
            lay->addWidget(note);
        }

        tabs->addTab(tab3, "📈  Frequency plot");
    }

    dlg->show();
}

// onSensitivityClicked
void MainWindow::onSensitivityClicked()
{
    QString workDir = QDir::currentPath();
    QString matPath = workDir + "/matrix.txt";
    QString freqPath = workDir + "/natural_frequencies.csv";
    QString sumPath = workDir + "/sensitivity_output.csv";
    QString curvePath = workDir + "/sensitivity_vs_freq.csv";

    // 1. Prerequisites
    if (!QFile::exists(matPath))
    {
        QMessageBox::warning(this, "Sensitivity",
                             "matrix.txt not found.\nPlease click 'Export Matrices' first.");
        return;
    }
    if (!QFile::exists(freqPath))
    {
        QMessageBox::warning(this, "Sensitivity",
                             "natural_frequencies.csv not found.\nRun 'Export Matrices' (which also runs qr_solver).");
        return;
    }

    QString solverPath =
        QCoreApplication::applicationDirPath() + "/sensitivity_solver.exe";
    if (!QFile::exists(solverPath))
    {
        QMessageBox::critical(this, "Sensitivity",
                              "sensitivity_solver.exe not found.\n\n"
                              "Add to CMakeLists.txt:\n\n"
                              "  add_executable(sensitivity_solver sensitivity.cpp)\n"
                              "  set_target_properties(sensitivity_solver PROPERTIES\n"
                              "    RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}\")");
        return;
    }

    // 2. Remove stale outputs
    QFile::remove(sumPath);
    QFile::remove(curvePath);

    // 3. Run solver
    sensitivityButton->setEnabled(false);
    sensitivityButton->setText("⏳ Analysing…");

    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(workDir);
    proc->start(solverPath);

    if (!proc->waitForStarted(3000))
    {
        sensitivityButton->setEnabled(true);
        sensitivityButton->setText("🎯 Sensitivity");
        QMessageBox::critical(this, "Sensitivity", "Could not start sensitivity_solver.exe.");
        delete proc;
        return;
    }

    connect(proc, &QProcess::finished, this,
            [this, proc, sumPath, curvePath](int, QProcess::ExitStatus)
            {
                sensitivityButton->setEnabled(true);
                sensitivityButton->setText("🎯 Sensitivity");
                proc->deleteLater();

                if (!QFile::exists(sumPath))
                {
                    QMessageBox::warning(this, "Sensitivity",
                                         "sensitivity_solver ran but sensitivity_output.csv was not created.");
                    return;
                }

                // 4. Parse results
                QVector<SensParam> params = parseSensitivitySummary(sumPath);
                QVector<double> omegas;
                if (QFile::exists(curvePath))
                    parseSensitivityFreqCurves(curvePath, params, omegas);

                // 5. Open dialog
                openSensitivityDialog(params, omegas);
            });
}