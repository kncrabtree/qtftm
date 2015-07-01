#include "pulsegeneratorsettingsdialog.h"
#include "ui_pulsegeneratorsettingsdialog.h"
#include <QSettings>

PulseGeneratorSettingsDialog::PulseGeneratorSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PulseGeneratorSettingsDialog), d_key(QString("pulseGenerator"))
{
    ui->setupUi(this);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    connect(ui->chEButton,&QAbstractButton::toggled,this,&PulseGeneratorSettingsDialog::toggleButtonText);
    connect(ui->chFButton,&QAbstractButton::toggled,this,&PulseGeneratorSettingsDialog::toggleButtonText);
    connect(ui->chGButton,&QAbstractButton::toggled,this,&PulseGeneratorSettingsDialog::toggleButtonText);
    connect(ui->chHButton,&QAbstractButton::toggled,this,&PulseGeneratorSettingsDialog::toggleButtonText);

    ui->repRateDoubleSpinBox->setValue(s.value(QString("repRate"),6.0).toDouble());

    ui->chAButton->setEnabled(false);
    ui->chAButton->setChecked(false);
    ui->chAButton->setText(QString("High"));
    ui->chANameLineEdit->setText(QString("Gas"));
    ui->chANameLineEdit->setEnabled(false);
    s.setValue(QString("chAName"),QString("Gas"));

    ui->chBButton->setEnabled(false);
    ui->chBButton->setChecked(false);
    ui->chBButton->setText(QString("High"));
    ui->chBNameLineEdit->setText(QString("DC"));
    ui->chBNameLineEdit->setEnabled(false);
    s.setValue(QString("chBName"),QString("DC"));

    ui->chCButton->setEnabled(false);
    ui->chCButton->setChecked(true);
    ui->chCButton->setText(QString("Low"));
    ui->chCNameLineEdit->setText(QString("MW"));
    ui->chCNameLineEdit->setEnabled(false);
    s.setValue(QString("chCName"),QString("MW"));

    ui->chDButton->setEnabled(false);
    ui->chDButton->setChecked(false);
    ui->chDButton->setText(QString("High"));
    ui->chDNameLineEdit->setText(QString("DR"));
    ui->chDNameLineEdit->setEnabled(false);
    s.setValue(QString("chDName"),QString("DR"));

    ui->chELineEdit->setText(s.value(QString("chEName"),QString("Aux1")).toString());
    if(s.value(QString("chEActive"),1).toInt() == 0)
        ui->chEButton->toggle();

    ui->chFLineEdit->setText(s.value(QString("chFName"),QString("Aux2")).toString());
    if(s.value(QString("chFActive"),1).toInt() == 0)
        ui->chFButton->toggle();

    ui->chGLineEdit->setText(s.value(QString("chGName"),QString("Aux3")).toString());
    if(s.value(QString("chGActive"),1).toInt() == 0)
        ui->chGButton->toggle();

    ui->chHLineEdit->setText(s.value(QString("chHName"),QString("Aux4")).toString());
    if(s.value(QString("chHActive"),1).toInt() == 0)
        ui->chHButton->toggle();

    s.endGroup();
    s.sync();

}

PulseGeneratorSettingsDialog::~PulseGeneratorSettingsDialog()
{
    delete ui;
}

void PulseGeneratorSettingsDialog::toggleButtonText(bool checked)
{
    QPushButton *b = static_cast<QPushButton*>(sender());
    if(b != nullptr)
    {
        if(checked)
            b->setText(QString("Low"));
        else
            b->setText(QString("High"));
    }
}

void PulseGeneratorSettingsDialog::accept()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    s.setValue(QString("repRate"),ui->repRateDoubleSpinBox->value());

    if(!ui->chELineEdit->text().isEmpty())
        s.setValue(QString("chEName"),ui->chELineEdit->text());
    else
        s.setValue(QString("chEName"),QString("Aux1"));
    if(ui->chEButton->isChecked())
        s.setValue(QString("chEActive"),0);
    else
        s.setValue(QString("chEActive"),1);

    if(!ui->chFLineEdit->text().isEmpty())
        s.setValue(QString("chFName"),ui->chFLineEdit->text());
    else
        s.setValue(QString("chFName"),QString("Aux2"));
    if(ui->chFButton->isChecked())
        s.setValue(QString("chFActive"),0);
    else
        s.setValue(QString("chFActive"),1);

    if(!ui->chGLineEdit->text().isEmpty())
        s.setValue(QString("chGName"),ui->chGLineEdit->text());
    else
        s.setValue(QString("chGName"),QString("Aux3"));
    if(ui->chGButton->isChecked())
        s.setValue(QString("chGActive"),0);
    else
        s.setValue(QString("chGActive"),1);

    if(!ui->chHLineEdit->text().isEmpty())
        s.setValue(QString("chHName"),ui->chHLineEdit->text());
    else
        s.setValue(QString("chHName"),QString("Aux4"));
    if(ui->chHButton->isChecked())
        s.setValue(QString("chHActive"),0);
    else
        s.setValue(QString("chHActive"),1);

    s.endGroup();
    s.sync();

    QDialog::accept();
}
