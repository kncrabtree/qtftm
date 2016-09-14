#include "singlescanwidget.h"
#include "ui_singlescanwidget.h"

#include <QSettings>
#include <QApplication>

SingleScanWidget::SingleScanWidget(QWidget *parent) :
     QWidget(parent),
     ui(new Ui::SingleScanWidget)
{
	ui->setupUi(this);

	ui->pulseConfigWidget->makeInternalConnections();

	connect(ui->dipoleDoubleSpinBox,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),[=](double d){
		if(d >= 0.01)
			ui->ssAttnSpinBox->setDisabled(true);
		else
			ui->ssAttnSpinBox->setEnabled(true);
	});

	ui->skipTuneCheckBox->setEnabled(false);

	//Set upper and lower limits on controls
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

	s.beginGroup(QString("ftmSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	ui->ssFtmDoubleSpinBox->setMinimum(s.value(QString("min"),5000.0).toDouble());
	ui->ssFtmDoubleSpinBox->setMaximum(s.value(QString("max"),43000.0).toDouble());
	s.endGroup();
	s.endGroup();

	s.beginGroup(QString("attn"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	ui->ssAttnSpinBox->setMinimum(s.value(QString("min"),0).toInt());
	ui->ssAttnSpinBox->setMaximum(s.value(QString("max"),70).toInt());
	s.endGroup();
	s.endGroup();

	s.beginGroup(QString("drSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	ui->ssDrDoubleSpinBox->setMinimum(s.value(QString("min"),1000.0).toDouble());
	ui->ssDrDoubleSpinBox->setMaximum(s.value(QString("max"),100000.0).toDouble());

	ui->ssPwrDoubleSpinBox->setMinimum(s.value(QString("minPower"),-30.0).toDouble());
	ui->ssPwrDoubleSpinBox->setMaximum(s.value(QString("maxPower"),17.0).toDouble());
	s.endGroup();
	s.endGroup();

	s.beginGroup(QString("hvps"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	ui->dcControlSpinBox->setMinimum(s.value(QString("min"),0).toInt());
	ui->dcControlSpinBox->setMaximum(s.value(QString("max"),2000).toInt());
	s.endGroup();
	s.endGroup();


	ui->ssShotsSpinBox->setValue(s.value(QString("lastScanShots"),100).toInt());
	ui->ssShotsSpinBox->setFocus();

	connect(ui->ssShotsSpinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,&SingleScanWidget::shotsChanged);
	connect(ui->magnetOnOffButton,&QAbstractButton::toggled,[=](bool on){
		if(on)
			ui->magnetOnOffButton->setText(QString("On"));
		else
			ui->magnetOnOffButton->setText(QString("Off"));
	});
    connect(ui->skipTuneCheckBox,&QCheckBox::toggled,ui->postTuneDelayBox,&QSpinBox::setDisabled);
}

SingleScanWidget::~SingleScanWidget()
{
	delete ui;
}

int SingleScanWidget::shots() const
{
	return ui->ssShotsSpinBox->value();
}

double SingleScanWidget::ftmFreq() const
{
	return ui->ssFtmDoubleSpinBox->value();
}

int SingleScanWidget::attn() const
{
	return ui->ssAttnSpinBox->value();
}

double SingleScanWidget::dipoleMoment() const
{
    return ui->dipoleDoubleSpinBox->value();
}

double SingleScanWidget::drFreq() const
{
	return ui->ssDrDoubleSpinBox->value();
}

double SingleScanWidget::drPower() const
{
    return ui->ssPwrDoubleSpinBox->value();
}

int SingleScanWidget::protectionTime() const
{
    return ui->pulseConfigWidget->protDelay();
}

int SingleScanWidget::scopeTime() const
{
    return ui->pulseConfigWidget->scopeDelay();
}


PulseGenConfig SingleScanWidget::pulseConfig() const
{
	return ui->pulseConfigWidget->getConfig();
}

int SingleScanWidget::dcVoltage() const
{
	return ui->dcControlSpinBox->value();
}

void SingleScanWidget::setFtmFreq(double d)
{
	ui->ssFtmDoubleSpinBox->setValue(d);
}

void SingleScanWidget::setAttn(int a)
{
	ui->ssAttnSpinBox->setValue(a);
}

void SingleScanWidget::setDrFreq(double d)
{
	ui->ssDrDoubleSpinBox->setValue(d);
}

void SingleScanWidget::setDrPower(double d)
{
	ui->ssPwrDoubleSpinBox->setValue(d);
}

void SingleScanWidget::setPulseConfig(const PulseGenConfig pc)
{
	ui->pulseConfigWidget->newConfig(pc);
}

void SingleScanWidget::setProtectionTime(int a)
{
    ui->pulseConfigWidget->newProtDelay(a);
}

void SingleScanWidget::setScopeTime(int a)
{
	ui->pulseConfigWidget->newScopeDelay(a);
}

void SingleScanWidget::setMagnet(bool b)
{
	ui->magnetOnOffButton->setChecked(b);
}

void SingleScanWidget::setDcVoltage(int v)
{
	ui->dcControlSpinBox->setValue(v);
}

void SingleScanWidget::enableSkipTune(bool enable)
{
	ui->skipTuneCheckBox->setEnabled(enable);
}

void SingleScanWidget::setFromScan(const Scan s)
{
    ui->ssShotsSpinBox->setValue(s.targetShots());
    setFtmFreq(s.ftFreq());
    setAttn(s.attenuation());
    setDrFreq(s.drFreq());
    setDrPower(s.drPower());
    setPulseConfig(s.pulseConfiguration());
    setProtectionTime(s.protectionDelayTime());
    setScopeTime(s.scopeDelayTime());
    ui->magnetOnOffButton->setChecked(s.magnet());
    ui->dipoleDoubleSpinBox->setValue(s.dipoleMoment());
    ui->dcControlSpinBox->setValue(s.dcVoltage());
    ui->skipTuneCheckBox->setChecked(s.skipTune());
    ui->postTuneDelayBox->setValue(s.postTuneDelayShots());
}

Scan SingleScanWidget::toScan() const
{
    Scan out;
    out.setTargetShots(shots());
    out.setFtFreq(ftmFreq());
    out.setAttenuation(attn());
    out.setDrFreq(drFreq());
    out.setDrPower(drPower());
    out.setPulseConfiguration(ui->pulseConfigWidget->getConfig());
    out.setProtectionDelayTime(ui->pulseConfigWidget->protDelay());
    out.setScopeDelayTime(ui->pulseConfigWidget->scopeDelay());
    out.setDipoleMoment(dipoleMoment());
    out.setMagnet(ui->magnetOnOffButton->isChecked());
    out.setDcVoltage(ui->dcControlSpinBox->value());
    out.setSkiptune(ui->skipTuneCheckBox->isChecked());
    if(!ui->skipTuneCheckBox->isChecked())
        out.setPostTuneDelayShots(ui->postTuneDelayBox->value());
    return out;
}

QSpinBox *SingleScanWidget::shotsSpinBox()
{
	return ui->ssShotsSpinBox;
}

void SingleScanWidget::setFtmSynthBoxEnabled(bool enabled)
{
	ui->ssFtmDoubleSpinBox->setEnabled(enabled);
}

void SingleScanWidget::setDrSynthBoxEnabled(bool enabled)
{
	ui->ssDrDoubleSpinBox->setEnabled(enabled);
}

void SingleScanWidget::setShotsBoxEnabled(bool enabled)
{
	ui->ssShotsSpinBox->setEnabled(enabled);
}

void SingleScanWidget::limitFtmRangeToDr()
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(QString("ftmSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	double ftMin = s.value(QString("min"),5000.0).toDouble();
	double ftMax = s.value(QString("max"),26000.0).toDouble();
	s.endGroup();
	s.endGroup();

	s.beginGroup(QString("drSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	double drMin = s.value(QString("min"),1000.0).toDouble();
	double drMax = s.value(QString("max"),26000.0).toDouble();
	s.endGroup();
	s.endGroup();

	double min = qMax(ftMin,drMin);
	double max = qMin(ftMax,drMax);

	ui->ssFtmDoubleSpinBox->setRange(min,max);
}



void SingleScanWidget::shotsChanged(int newShots)
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.setValue(QString("lastScanShots"),newShots);
	s.sync();
}



