#include "loadbatchdialog.h"
#include "ui_loadbatchdialog.h"
#include <QSettings>

LoadBatchDialog::LoadBatchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadBatchDialog)
{
    ui->setupUi(this);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    int surveyMax = s.value(QString("surveyNum"),0).toInt()-1;
    if(surveyMax > 0)
    {
	    ui->surveySpinBox->setSpecialValueText(QString(""));
        ui->surveySpinBox->setRange(1,surveyMax);
        ui->surveySpinBox->setValue(surveyMax);
    }
    else
    {
        ui->surveySpinBox->setRange(0,0);
        ui->surveySpinBox->setValue(0);
        ui->surveyButton->setChecked(false);
        ui->surveyButton->setCheckable(false);
        ui->drScanButton->setChecked(true);
    }

    int drMax = s.value(QString("drNum"),0).toInt()-1;
    if(drMax > 0)
    {
	    ui->drScanSpinBox->setSpecialValueText(QString(""));
        ui->drScanSpinBox->setRange(1,drMax);
        ui->drScanSpinBox->setValue(drMax);
    }
    else
    {
        ui->drScanSpinBox->setRange(0,0);
        ui->drScanSpinBox->setValue(0);
        ui->drScanButton->setChecked(false);
        ui->drScanButton->setCheckable(false);
        if(!ui->surveyButton->isChecked())
            ui->batchButton->setChecked(true);
    }

    int batchMax = s.value(QString("batchNum"),0).toInt()-1;
    if(batchMax > 0)
    {
	    ui->batchSpinBox->setSpecialValueText(QString(""));
        ui->batchSpinBox->setRange(1,batchMax);
        ui->batchSpinBox->setValue(batchMax);
    }
    else
    {
        ui->batchSpinBox->setRange(0,0);
        ui->batchSpinBox->setValue(0);
        ui->batchButton->setChecked(false);
        ui->batchButton->setCheckable(false);
        if(!ui->surveyButton->isChecked() && !ui->drScanButton->isChecked())
            ui->attenuationButton->setChecked(true);
    }

    int attenMax = s.value(QString("batchAttnNum"),0).toInt()-1;
    if(attenMax > 0)
    {
	    ui->attenuationSpinBox->setSpecialValueText(QString(""));
        ui->attenuationSpinBox->setRange(1,attenMax);
        ui->attenuationSpinBox->setValue(attenMax);
    }
    else
    {
        ui->attenuationSpinBox->setRange(0,0);
        ui->attenuationSpinBox->setValue(0);
        ui->attenuationButton->setChecked(false);
        ui->attenuationButton->setCheckable(false);
    }

    int drCorrMax = s.value(QString("drCorrNum"),0).toInt()-1;
    if(drCorrMax > 0)
    {
	    ui->drCorrSpinBox->setSpecialValueText(QString(""));
	    ui->drCorrSpinBox->setRange(1,drCorrMax);
	    ui->drCorrSpinBox->setValue(drCorrMax);
    }
    else
    {
	    ui->drCorrSpinBox->setRange(0,0);
	    ui->drCorrSpinBox->setValue(0);
	    ui->drCorrButton->setChecked(false);
	    ui->drCorrButton->setCheckable(false);
    }

    int catMax = s.value(QString("catTestNum"),0).toInt()-1;
    if(catMax > 0)
    {
	    ui->catSpinBox->setSpecialValueText(QString(""));
	    ui->catSpinBox->setRange(1,catMax);
	    ui->catSpinBox->setValue(catMax);
    }
    else
    {
	    ui->catSpinBox->setRange(0,0);
	    ui->catSpinBox->setValue(0);
	    ui->catButton->setChecked(false);
	    ui->catButton->setCheckable(false);
    }

    if(ui->surveyButton->isChecked())
        ui->surveySpinBox->setEnabled(true);
    else
        ui->surveySpinBox->setEnabled(false);

    if(ui->drScanButton->isChecked())
        ui->drScanSpinBox->setEnabled(true);
    else
        ui->drScanSpinBox->setEnabled(false);

    if(ui->batchButton->isChecked())
        ui->batchSpinBox->setEnabled(true);
    else
        ui->batchSpinBox->setEnabled(false);

    if(ui->attenuationButton->isChecked())
        ui->attenuationSpinBox->setEnabled(true);
    else
        ui->attenuationSpinBox->setEnabled(false);

    if(ui->drCorrButton->isChecked())
	    ui->drCorrSpinBox->setEnabled(true);
    else
	    ui->drCorrSpinBox->setEnabled(false);

    if(ui->catButton->isChecked())
	    ui->catSpinBox->setEnabled(true);
    else
	    ui->catSpinBox->setEnabled(false);


    connect(ui->surveyButton,&QAbstractButton::toggled,ui->surveySpinBox,&QWidget::setEnabled);
    connect(ui->drScanButton,&QAbstractButton::toggled,ui->drScanSpinBox,&QWidget::setEnabled);
    connect(ui->batchButton,&QAbstractButton::toggled,ui->batchSpinBox,&QWidget::setEnabled);
    connect(ui->attenuationButton,&QAbstractButton::toggled,ui->attenuationSpinBox,&QWidget::setEnabled);
    connect(ui->drCorrButton,&QAbstractButton::toggled,ui->drCorrSpinBox,&QWidget::setEnabled);
    connect(ui->catButton,&QAbstractButton::toggled,ui->catSpinBox,&QWidget::setEnabled);

    ui->removeDCCheckBox->setChecked(true);
}

LoadBatchDialog::~LoadBatchDialog()
{
    delete ui;
}

QPair<QtFTM::BatchType, int> LoadBatchDialog::selection() const
{
    QPair<QtFTM::BatchType,int> out(QtFTM::SingleScan,0);

    if(ui->surveyButton->isChecked())
    {
	   out.first = QtFTM::Survey;
        out.second = ui->surveySpinBox->value();
    }
    else if(ui->drScanButton->isChecked())
    {
	   out.first = QtFTM::DrScan;
        out.second = ui->drScanSpinBox->value();
    }
    else if(ui->batchButton->isChecked())
    {
	   out.first = QtFTM::Batch;
        out.second = ui->batchSpinBox->value();
    }
    else if(ui->attenuationButton->isChecked())
    {
	   out.first = QtFTM::Attenuation;
        out.second = ui->attenuationSpinBox->value();
    }
    else if(ui->drCorrButton->isChecked())
    {
	    out.first = QtFTM::DrCorrelation;
	    out.second = ui->drCorrSpinBox->value();
    }
    else if(ui->catButton->isChecked())
    {
	    out.first = QtFTM::Categorize;
	    out.second = ui->catSpinBox->value();
    }

    return out;
}

double LoadBatchDialog::delay() const
{
    return ui->delayDoubleSpinBox->value();
}

int LoadBatchDialog::hpf() const
{
    return ui->highPassFilterSpinBox->value();
}

double LoadBatchDialog::exp() const
{
	return ui->exponentialFilterDoubleSpinBox->value();
}

bool LoadBatchDialog::removeDC() const
{
	return ui->removeDCCheckBox->isChecked();
}

bool LoadBatchDialog::padFid() const
{
	return ui->zeroPadFIDsCheckBox->isChecked();
}
