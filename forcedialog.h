#ifndef FORCEDIALOG_H
#define FORCEDIALOG_H

#include <QDialog>
#include "forcefunction.h"

class QLineEdit;
class QDoubleSpinBox;
class QLabel;
class PreviewPlot;

class ForceDialog : public QDialog
{
    Q_OBJECT

public:
    // Pass existing force to pre-populate fields; pass empty ForceFunction for new force
    explicit ForceDialog(const ForceFunction &current, QWidget *parent = nullptr);

    ForceFunction result() const { return m_result; }

private slots:
    void onPreview();
    void onAccept();

private:
    QLineEdit      *m_exprEdit;
    QDoubleSpinBox *m_omegaSpin;
    QDoubleSpinBox *m_angleSpin;
    QDoubleSpinBox *m_tMaxSpin;
    QLabel         *m_errorLabel;
    PreviewPlot    *m_plot;

    ForceFunction   m_result;
};

#endif