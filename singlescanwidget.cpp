#include "singlescanwidget.h"
#include <QSettings>
#include <QApplication>

SingleScanWidget::SingleScanWidget(QWidget *parent) :
     QWidget(parent),
     ui(new Ui::SingleScanWidget)
{
	ui->setupUi(this);

	connect(ui->ssDcPulseOnButton,&QAbstractButton::toggled,this,&SingleScanWidget::toggleOnOffButtonText);
	connect(ui->ssDrPulseOnButton,&QAbstractButton::toggled,this,&SingleScanWidget::toggleOnOffButtonText);
    connect(ui->ssAux1PulseOnButton,&QAbstractButton::toggled,this,&SingleScanWidget::toggleOnOffButtonText);
	connect(ui->ssAux2PulseOnButton,&QAbstractButton::toggled,this,&SingleScanWidget::toggleOnOffButtonText);
	connect(ui->ssAux3PulseOnButton,&QAbstractButton::toggled,this,&SingleScanWidget::toggleOnOffButtonText);
	connect(ui->ssAux4PulseOnButton,&QAbstractButton::toggled,this,&SingleScanWidget::toggleOnOffButtonText);
	connect(ui->magnetOnOffButton,&QAbstractButton::toggled,this,&SingleScanWidget::toggleOnOffButtonText);
	connect(ui->dipoleDoubleSpinBox,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),[=](double d){
		if(d >= 0.01)
			ui->ssAttnSpinBox->setDisabled(true);
		else
			ui->ssAttnSpinBox->setEnabled(true);
	});

	ui->skipTuneCheckBox->setEnabled(false);

	//Set upper and lower limits on controls
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

	ui->ssFtmDoubleSpinBox->setMinimum(s.value(QString("ftmSynth/min"),5000.0).toDouble());
	ui->ssFtmDoubleSpinBox->setMaximum(s.value(QString("ftmSynth/max"),43000.0).toDouble());

	ui->ssAttnSpinBox->setMinimum(s.value(QString("attenuators/min"),0).toInt());
	ui->ssAttnSpinBox->setMaximum(s.value(QString("attenuators/max"),70).toInt());
#ifndef CONFIG_NODRSYNTH
	ui->ssDrDoubleSpinBox->setMinimum(s.value(QString("drSynth/min"),1000.0).toDouble());
	ui->ssDrDoubleSpinBox->setMaximum(s.value(QString("drSynth/max"),100000.0).toDouble());

	ui->ssPwrDoubleSpinBox->setMinimum(s.value(QString("drSynth/minPower"),-30.0).toDouble());
	ui->ssPwrDoubleSpinBox->setMaximum(s.value(QString("drSynth/maxPower"),17.0).toDouble());
#else
    ui->ssDrDoubleSpinBox->setMinimum(-10.0);
    ui->ssDrDoubleSpinBox->setValue(-1.0);
    ui->ssPwrDoubleSpinBox->setValue(0.0);
    ui->ssDrDoubleSpinBox->setEnabled(false);
    ui->ssPwrDoubleSpinBox->setEnabled(false);
#endif

	double pGenMinWidth = s.value(QString("pulseGenerator/minWidth"),0.004).toDouble();
	double pGenMaxWidth = s.value(QString("pulseGenerator/maxWidth"),100000.000).toDouble();
	double pGenMinDelay = s.value(QString("pulseGenerator/minDelay"),0.0).toDouble();
	double pGenMaxDelay = s.value(QString("pulseGenerator/maxDelay"),100000.000).toDouble();

	ui->ssGasWidthBox->setMinimum(pGenMinWidth);
	ui->ssGasWidthBox->setMaximum(pGenMaxWidth);
	ui->ssGasDelayBox->setMinimum(pGenMinDelay);
	ui->ssGasDelayBox->setMaximum(pGenMaxDelay);

	ui->ssDcWidthBox->setMinimum(pGenMinWidth);
	ui->ssDcWidthBox->setMaximum(pGenMaxWidth);
	ui->ssDcDelayBox->setMinimum(pGenMinDelay);
	ui->ssDcDelayBox->setMaximum(pGenMaxDelay);

	ui->ssMwWidthBox->setMinimum(pGenMinWidth);
	ui->ssMwWidthBox->setMaximum(pGenMaxWidth);
	ui->ssMwDelayBox->setMinimum(pGenMinDelay);
	ui->ssMwDelayBox->setMaximum(pGenMaxDelay);

	ui->ssDrWidthBox->setMinimum(pGenMinWidth);
	ui->ssDrWidthBox->setMaximum(pGenMaxWidth);
	ui->ssDrDelayBox->setMinimum(pGenMinDelay);
	ui->ssDrDelayBox->setMaximum(pGenMaxDelay);

    ui->ssMonWidthBox->setMinimum(pGenMinWidth);
    ui->ssMonWidthBox->setMaximum(pGenMaxWidth);
    ui->ssMonDelayBox->setMinimum(pGenMinDelay);    // ssMonDelayBox changed from readOnly to not readOnly Aug 15 2014 praa - checking to see if this is recorded
    ui->ssMonDelayBox->setMaximum(pGenMaxDelay);

	ui->ssAux2WidthBox->setMinimum(pGenMinWidth);
	ui->ssAux2WidthBox->setMaximum(pGenMaxWidth);
	ui->ssAux2DelayBox->setMinimum(pGenMinDelay);
	ui->ssAux2DelayBox->setMaximum(pGenMaxDelay);

	ui->ssAux3WidthBox->setMinimum(pGenMinWidth);
	ui->ssAux3WidthBox->setMaximum(pGenMaxWidth);
	ui->ssAux3DelayBox->setMinimum(pGenMinDelay);
	ui->ssAux3DelayBox->setMaximum(pGenMaxDelay);

	ui->ssAux4WidthBox->setMinimum(pGenMinWidth);
	ui->ssAux4WidthBox->setMaximum(pGenMaxWidth);
	ui->ssAux4DelayBox->setMinimum(pGenMinDelay);
	ui->ssAux4DelayBox->setMaximum(pGenMaxDelay);

    ui->chELabel->setText(s.value(QString("pulseGenerator/chEName"),QString("Aux1")).toString());
    ui->chFLabel->setText(s.value(QString("pulseGenerator/chFName"),QString("Aux2")).toString());
    ui->chGLabel->setText(s.value(QString("pulseGenerator/chGName"),QString("Aux3")).toString());
    ui->chHLabel->setText(s.value(QString("pulseGenerator/chHName"),QString("Aux4")).toString());

	ui->ssShotsSpinBox->setValue(s.value(QString("lastScanShots"),100).toInt());
	ui->ssShotsSpinBox->setFocus();

	connect(ui->ssShotsSpinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,&SingleScanWidget::shotsChanged);
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
    return ui->ssProtectionSpinBox->value();
}

int SingleScanWidget::scopeTime() const
{
    return ui->ssScopeDelaySpinBox->value();
}


QList<PulseGenerator::PulseChannelConfiguration> SingleScanWidget::pulseConfig() const
{
	if(pConfig.isEmpty())
		return pConfig;

    QList<PulseGenerator::PulseChannelConfiguration> out = pConfig;

    out[0].delay = ui->ssGasDelayBox->value();
    out[0].width = ui->ssGasWidthBox->value();

    out[1].delay = ui->ssDcDelayBox->value();
    out[1].width = ui->ssDcWidthBox->value();
    out[1].enabled = ui->ssDcPulseOnButton->isChecked();

    out[2].delay = ui->ssMwDelayBox->value();
    out[2].width = ui->ssMwWidthBox->value();

    out[3].delay = ui->ssDrDelayBox->value();
    out[3].width = ui->ssDrWidthBox->value();
    out[3].enabled = ui->ssDrPulseOnButton->isChecked();

    out[4].delay = ui->ssMonDelayBox->value();
    out[4].width = ui->ssMonWidthBox->value();
    out[4].enabled = ui->ssAux1PulseOnButton->isChecked();

    out[5].delay = ui->ssAux2DelayBox->value();
    out[5].width = ui->ssAux2WidthBox->value();
    out[5].enabled = ui->ssAux2PulseOnButton->isChecked();

    out[6].delay = ui->ssAux3DelayBox->value();
    out[6].width = ui->ssAux3WidthBox->value();
    out[6].enabled = ui->ssAux3PulseOnButton->isChecked();

    out[7].delay = ui->ssAux4DelayBox->value();
    out[7].width = ui->ssAux4WidthBox->value();
    out[7].enabled = ui->ssAux4PulseOnButton->isChecked();

    return out;

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



void SingleScanWidget::setPulseConfig(const QList<PulseGenerator::PulseChannelConfiguration> pc)
{
	if(pc.size() != 8)
		return;

	pConfig = pc;

	//first is gas pulse
	ui->ssGasDelayBox->setValue(pc[0].delay);
	ui->ssGasWidthBox->setValue(pc[0].width);

	//then DC
	ui->ssDcDelayBox->setValue(pc[1].delay);
	ui->ssDcWidthBox->setValue(pc[1].width);
	ui->ssDcPulseOnButton->setChecked(pc[1].enabled);

	//then MW pulse
	ui->ssMwDelayBox->setValue(pc[2].delay);
	ui->ssMwWidthBox->setValue(pc[2].width);

	//then DR
	ui->ssDrDelayBox->setValue(pc[3].delay);
	ui->ssDrWidthBox->setValue(pc[3].width);
	ui->ssDrPulseOnButton->setChecked(pc[3].enabled);

	//then aux 1-4
    ui->ssMonDelayBox->setValue(pc[4].delay);
    ui->ssMonWidthBox->setValue(pc[4].width);
    ui->ssAux1PulseOnButton->setChecked(pc[4].enabled);

	ui->ssAux2DelayBox->setValue(pc[5].delay);
	ui->ssAux2WidthBox->setValue(pc[5].width);
	ui->ssAux2PulseOnButton->setChecked(pc[5].enabled);

	ui->ssAux3DelayBox->setValue(pc[6].delay);
	ui->ssAux3WidthBox->setValue(pc[6].width);
	ui->ssAux3PulseOnButton->setChecked(pc[6].enabled);

	ui->ssAux4DelayBox->setValue(pc[7].delay);
	ui->ssAux4WidthBox->setValue(pc[7].width);
	ui->ssAux4PulseOnButton->setChecked(pc[7].enabled);

}

void SingleScanWidget::setProtectionTime(int a)
{
    ui->ssProtectionSpinBox->setValue(a);
}

void SingleScanWidget::setScopeTime(int a)
{
	ui->ssScopeDelaySpinBox->setValue(a);
}

void SingleScanWidget::setMagnet(bool b)
{
	ui->magnetOnOffButton->setChecked(b);
}

void SingleScanWidget::enableSkipTune(bool enable)
{
	ui->skipTuneCheckBox->setEnabled(enable);
}

void SingleScanWidget::setFromScan(Scan s)
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
}

Scan SingleScanWidget::toScan() const
{
    Scan out;
    out.setTargetShots(shots());
    out.setFtFreq(ftmFreq());
    out.setAttenuation(attn());
    out.setDrFreq(drFreq());
    out.setDrPower(drPower());
    out.setPulseConfiguration(pulseConfig());
    out.setProtectionDelayTime(protectionTime());
    out.setScopeDelayTime(scopeTime());
    out.setDipoleMoment(dipoleMoment());
    out.setMagnet(ui->magnetOnOffButton->isChecked());
    out.setSkiptune(ui->skipTuneCheckBox->isChecked());
    return out;
}


void SingleScanWidget::toggleOnOffButtonText(bool on)
{
	QPushButton *pb = qobject_cast<QPushButton*>(sender());
	if(pb)
	{
		if(on)
			pb->setText(QString("On"));
		else
			pb->setText(QString("Off"));
	}
}

void SingleScanWidget::shotsChanged(int newShots)
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.setValue(QString("lastScanShots"),newShots);
	s.sync();
}



