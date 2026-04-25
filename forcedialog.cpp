#include "forcedialog.h"
#include "forcefunction.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QPainter>
#include <QWidget>
#include <QGroupBox>
#include <QPainterPath>
#include <cmath>
#include <limits>

// ------------------------------------------------------------------------------
// Inline preview plot — draws F(t) over [0, tMax]
// ------------------------------------------------------------------------------
class PreviewPlot : public QWidget
{
public:
    explicit PreviewPlot(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(400, 180);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setStyleSheet("background:#1e1e2e; border:1px solid #45475a; border-radius:4px;");
    }

    void setData(const QVector<double> &t, const QVector<double> &f)
    {
        m_t = t;
        m_f = f;
        update();
    }

    void setError(const QString &err)
    {
        m_error = err;
        m_t.clear();
        m_f.clear();
        update();
    }
    void clearError() { m_error.clear(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int pad = 36;
        QRectF plot(pad, 8, width() - pad - 8, height() - 24);

        // Background already set by stylesheet; draw axis frame
        p.setPen(QPen(QColor("#45475a"), 1));
        p.drawRect(plot);

        if (!m_error.isEmpty())
        {
            p.setPen(QColor("#f38ba8"));
            p.setFont(QFont("Arial", 9));
            p.drawText(plot, Qt::AlignCenter | Qt::TextWordWrap, m_error);
            return;
        }

        if (m_t.size() < 2)
        {
            p.setPen(QColor("#6c7086"));
            p.setFont(QFont("Arial", 9));
            p.drawText(plot, Qt::AlignCenter, "Press Preview to plot");
            return;
        }

        // Find data range
        double fMin = *std::min_element(m_f.begin(), m_f.end());
        double fMax = *std::max_element(m_f.begin(), m_f.end());
        if (fMax == fMin)
        {
            fMax += 1.0;
            fMin -= 1.0;
        }
        double tMin = m_t.first(), tMax = m_t.last();

        auto mapX = [&](double t) -> double
        {
            return plot.left() + (t - tMin) / (tMax - tMin) * plot.width();
        };
        auto mapY = [&](double f) -> double
        {
            return plot.bottom() - (f - fMin) / (fMax - fMin) * plot.height();
        };

        // Zero line
        if (fMin < 0 && fMax > 0)
        {
            p.setPen(QPen(QColor("#45475a"), 1, Qt::DashLine));
            double y0 = mapY(0);
            p.drawLine(QPointF(plot.left(), y0), QPointF(plot.right(), y0));
        }

        // Curve
        p.setPen(QPen(QColor("#f9e2af"), 2.0));
        QPainterPath path;
        path.moveTo(mapX(m_t[0]), mapY(m_f[0]));
        for (int i = 1; i < m_t.size(); ++i)
            path.lineTo(mapX(m_t[i]), mapY(m_f[i]));
        p.drawPath(path);

        // Axis labels
        p.setPen(QColor("#cdd6f4"));
        p.setFont(QFont("Arial", 8));

        // Y-axis: min, max, zero if visible
        p.drawText(QRectF(0, plot.top() - 6, pad - 2, 14),
                   Qt::AlignRight, QString::number(fMax, 'g', 3));
        p.drawText(QRectF(0, plot.bottom() - 6, pad - 2, 14),
                   Qt::AlignRight, QString::number(fMin, 'g', 3));

        // X-axis: start and end
        p.drawText(QRectF(plot.left() - 10, plot.bottom() + 4, 40, 14),
                   Qt::AlignLeft, "0");
        p.drawText(QRectF(plot.right() - 30, plot.bottom() + 4, 40, 14),
                   Qt::AlignLeft, QString::number(tMax, 'g', 3) + "s");

        // Units label
        p.setPen(QColor("#6c7086"));
        p.drawText(QRectF(plot.left(), plot.top() - 16, 60, 14),
                   Qt::AlignLeft, "F (N)");
    }

private:
    QVector<double> m_t, m_f;
    QString m_error;
};

// ------------------------------------------------------------------------------
// ForceDialog
// ------------------------------------------------------------------------------
ForceDialog::ForceDialog(const ForceFunction &current, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Define Force Function");
    setMinimumWidth(480);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setSpacing(10);

    //  Expression hints -------------------------------------------------------
    QLabel *hint = new QLabel(
        "<b>Available variables:</b> &nbsp; <tt>t</tt> (time, s) &nbsp; "
        "<tt>w</tt> (ω, rad/s) &nbsp; <tt>pi</tt> &nbsp; <tt>e</tt><br>"
        "<b>Functions:</b> &nbsp; <tt>sin cos tan exp log sqrt abs pow</tt><br>"
        "<b>Examples:</b> &nbsp; <tt>10*sin(w*t)</tt> &nbsp; "
        "<tt>3*exp(-5*t)</tt> &nbsp; <tt>5*(t&lt;2 ? t : 2)</tt>");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#a6adc8; font-size:11px; "
                        "background:#181825; padding:6px; border-radius:4px;");
    root->addWidget(hint);

    // Parameters form /---------------------------------------------------------------
    QFormLayout *form = new QFormLayout;
    form->setSpacing(6);

    m_exprEdit = new QLineEdit(current.isEmpty() ? "10*sin(w*t)" : current.expression());
    m_exprEdit->setPlaceholderText("e.g.  10*sin(w*t)  or  3*exp(-5*t)");
    m_exprEdit->setFont(QFont("Courier New", 10));
    form->addRow("F(t) =", m_exprEdit);

    m_omegaSpin = new QDoubleSpinBox;
    m_omegaSpin->setRange(0.001, 1e6);
    m_omegaSpin->setDecimals(4);
    m_omegaSpin->setSuffix(" rad/s");
    m_omegaSpin->setValue(current.isEmpty() ? 1.0 : current.omega());
    form->addRow("ω (omega):", m_omegaSpin);

    m_angleSpin = new QDoubleSpinBox;
    m_angleSpin->setRange(-360.0, 360.0);
    m_angleSpin->setDecimals(1);
    m_angleSpin->setSuffix(" °");
    m_angleSpin->setValue(current.isEmpty() ? 0.0 : current.angleDeg());
    QLabel *angleNote = new QLabel("  (0° = right, 90° = up, 180° = left, 270° = down)");
    angleNote->setStyleSheet("color:#6c7086; font-size:10px;");
    QHBoxLayout *angleRow = new QHBoxLayout;
    angleRow->addWidget(m_angleSpin);
    angleRow->addWidget(angleNote);
    form->addRow("Direction:", angleRow);

    m_tMaxSpin = new QDoubleSpinBox;
    m_tMaxSpin->setRange(0.1, 1000.0);
    m_tMaxSpin->setDecimals(2);
    m_tMaxSpin->setSuffix(" s");
    m_tMaxSpin->setValue(10.0);
    form->addRow("Preview t-range:", m_tMaxSpin);

    root->addLayout(form);

    // Error label ------------------------------------------------
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color:#f38ba8; font-size:11px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    //  Preview button + plot  --------------------------------------
    QPushButton *previewBtn = new QPushButton("▶  Preview");
    previewBtn->setFixedHeight(28);
    root->addWidget(previewBtn);

    m_plot = new PreviewPlot(this);
    root->addWidget(m_plot);

    // OK / Cancel -------------------------------------------------
    QDialogButtonBox *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    root->addWidget(btns);

    connect(previewBtn, &QPushButton::clicked, this, &ForceDialog::onPreview);
    connect(btns, &QDialogButtonBox::accepted, this, &ForceDialog::onAccept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Auto-preview existing expression on open
    if (!current.isEmpty())
        onPreview();
}

void ForceDialog::onPreview()
{
    const QString expr = m_exprEdit->text().trimmed();
    const double omega = m_omegaSpin->value();
    const double tMax = m_tMaxSpin->value();

    QString err = ForceFunction::validate(expr, omega);
    if (!err.isEmpty())
    {
        m_errorLabel->setText("Error: " + err);
        m_errorLabel->show();
        m_plot->setError("Invalid expression:\n" + err);
        return;
    }
    m_errorLabel->hide();
    m_plot->clearError();

    ForceFunction fn(expr, m_angleSpin->value(), omega);
    const int N = 400;
    QVector<double> ts(N), fs(N);
    for (int i = 0; i < N; ++i)
    {
        double t = tMax * i / (N - 1);
        ts[i] = t;
        fs[i] = fn.evaluate(t);
    }
    m_plot->setData(ts, fs);
}

void ForceDialog::onAccept()
{
    const QString expr = m_exprEdit->text().trimmed();
    const double omega = m_omegaSpin->value();

    QString err = ForceFunction::validate(expr, omega);
    if (!err.isEmpty())
    {
        m_errorLabel->setText("Error: " + err);
        m_errorLabel->show();
        return; // don't close — let user fix it
    }

    m_result = ForceFunction(expr, m_angleSpin->value(), omega);
    accept();
}