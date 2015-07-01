#ifndef PULSEGENERATORSETTINGSDIALOG_H
#define PULSEGENERATORSETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class PulseGeneratorSettingsDialog;
}

class PulseGeneratorSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PulseGeneratorSettingsDialog(QWidget *parent = nullptr);
    ~PulseGeneratorSettingsDialog();

public slots:
    void toggleButtonText(bool checked);
    void accept();
    
private:
    Ui::PulseGeneratorSettingsDialog *ui;

    QString d_key;
};

#endif // PULSEGENERATORSETTINGSDIALOG_H
