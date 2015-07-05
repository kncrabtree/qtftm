#include "drsetuppage.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <math.h>
#include "batchwizard.h"
#include <QSettings>

DrSetupPage::DrSetupPage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Enter DR scan parameters"));
	setSubTitle(QString("Fill in the DR scan parameters. You will be able to configure the details of the scan and the integration ranges on the following pages."));

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(QString("drSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	double synthMin = s.value(QString("min"),1000.0).toDouble();
	double synthMax = s.value(QString("max"),95000.0).toDouble();
	s.endGroup();
	s.endGroup();

	QVBoxLayout *vl = new QVBoxLayout(this);

	QGroupBox *paramsBox = new QGroupBox(QString("Parameters"),this);
	QFormLayout *pBoxLayout = new QFormLayout(paramsBox);

	startBox = new QDoubleSpinBox(this);
	startBox->setObjectName(QString("drStartBox"));
	startBox->setDecimals(3);
	startBox->setMinimum(synthMin);
	startBox->setMaximum(synthMax);
	startBox->setSingleStep(100.0);
	startBox->setSuffix(QString(" MHz"));
    startBox->setValue(synthMin);
	startBox->setToolTip(QString("DR frequency at which the scan will begin"));
	startBox->setKeyboardTracking(false);

	stopBox = new QDoubleSpinBox(this);
	stopBox->setObjectName(QString("drStopBox"));
	stopBox->setDecimals(3);
	stopBox->setMinimum(synthMin);
	stopBox->setMaximum(synthMax);
	stopBox->setSingleStep(100.0);
	stopBox->setSuffix(QString(" MHz"));
    stopBox->setValue(synthMax);
    stopBox->setToolTip(QString("DR frequency at which the scan will end."));
	stopBox->setKeyboardTracking(false);

	stepBox = new QSpinBox(this);
	stepBox->setObjectName(QString("drStepBox"));
	stepBox->setMinimum(1);
	stepBox->setMaximum(100000);
	stepBox->setSingleStep(25);
	stepBox->setSuffix(QString(" kHz"));
	stepBox->setValue(50);
	stepBox->setToolTip(QString("Absolute value of step size (sign is determined from start and stop frequencies)."));
	stepBox->setKeyboardTracking(false);

    scansBetweenTuningVoltageReadings = new QSpinBox(this);
    scansBetweenTuningVoltageReadings->setObjectName(QString("drScansBetweenTuningVoltageReadings"));
    scansBetweenTuningVoltageReadings->setMinimum(0);
    scansBetweenTuningVoltageReadings->setMaximum(100000);
    scansBetweenTuningVoltageReadings->setSingleStep(1);
    scansBetweenTuningVoltageReadings->setSuffix(QString(" scans"));
    scansBetweenTuningVoltageReadings->setValue(0);
    scansBetweenTuningVoltageReadings->setToolTip(QString("Zero means every scan will read it. one=every other scan, etc."));
    scansBetweenTuningVoltageReadings->setKeyboardTracking(false);

	calCheckBox = new QCheckBox(this);
    calCheckBox->setChecked(false);
	calCheckBox->setToolTip(QString("When checked, a background scan will be taken between each point, and the DR signal will be ratioed"));

	//when the boxes are changed, update the label
	auto doubleVc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
	auto intVc = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);

	connect(startBox,doubleVc,this,&DrSetupPage::updateLabel);
	connect(stopBox,doubleVc,this,&DrSetupPage::updateLabel);
	connect(stepBox,intVc,this,&DrSetupPage::updateLabel);
    connect(scansBetweenTuningVoltageReadings,intVc,this,&DrSetupPage::updateLabel);
	connect(calCheckBox,&QAbstractButton::toggled,this,&DrSetupPage::updateLabel);

	pBoxLayout->addRow(QString("Start"),startBox);
	pBoxLayout->addRow(QString("Stop"),stopBox);
	pBoxLayout->addRow(QString("Step size"),stepBox);
    pBoxLayout->addRow(QString("Scans between Tuning Voltage Reads (0=read every scan)"),scansBetweenTuningVoltageReadings);
	pBoxLayout->addRow(QString("Calibration?"),calCheckBox);

	paramsBox->setLayout(pBoxLayout);

	vl->addWidget(paramsBox);

	//calculate number of scans
	int scans = (int)ceil(fabs(stopBox->value()-startBox->value())/(double)stepBox->value()*1000.0)+1.0;
	int calScans = 0;
	if(calCheckBox->isChecked())
		calScans = scans;

	label = new QLabel(QString("%1 DR scans, %2 Calibration scans").arg(scans).arg(calScans));

	vl->addWidget(label);

	setLayout(vl);

	//register fields for later use
	registerField(QString("drStart"),startBox);
	registerField(QString("drStop"),stopBox);
	registerField(QString("drStep"),stepBox);
    registerField(QString("drScansBetweenTuningVoltageReadings"),scansBetweenTuningVoltageReadings);
	registerField(QString("drCal"),calCheckBox);
}

int DrSetupPage::nextId() const
{
    return BatchWizard::Page_DrScanSetup;
}

bool DrSetupPage::validatePage()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("wizard"));
    s.setValue(QString("drStart"),startBox->value());
    s.setValue(QString("drStop"),stopBox->value());
    s.setValue(QString("drStep"),stepBox->value());
    s.setValue(QString("drTuneSteps"),scansBetweenTuningVoltageReadings->value());
    s.setValue(QString("drCal"),calCheckBox->isChecked());
    s.endGroup();
    s.sync();

    return true;
}

void DrSetupPage::initializePage()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("wizard"));

    startBox->setValue(s.value(QString("drStart"),startBox->value()).toDouble());
    stopBox->setValue(s.value(QString("drStop"),stopBox->value()).toDouble());
    stepBox->setValue(s.value(QString("drStep"),stepBox->value()).toDouble());
    scansBetweenTuningVoltageReadings->setValue(s.value(QString("drTuneSteps"),
                               scansBetweenTuningVoltageReadings->value()).toInt());
    calCheckBox->setChecked(s.value(QString("drCal"),calCheckBox->isChecked()).toBool());

    s.endGroup();
}

void DrSetupPage::updateLabel()
{
	if(fabs(startBox->value() - stopBox->value())<0.001)
	{
		//can't let the user make an invalid survey. force it back to a reasonable value
		if(sender() == startBox)
		{
			startBox->blockSignals(true);
			startBox->setValue(stopBox->value()-stepBox->value()/1000.0);
			startBox->blockSignals(false);
		}
		else
		{
			stopBox->blockSignals(true);
			stopBox->setValue(startBox->value()+stepBox->value()/1000.0);
			stopBox->blockSignals(false);
		}
	}

	//calculate number of scans
	int scans = (int)ceil(fabs(stopBox->value()-startBox->value())/(double)stepBox->value()*1000.0)+1.0;
	int calScans = 0;
	if(calCheckBox->isChecked())
		calScans = scans;

	label->setText(QString("%1 DR scans, %2 Calibration scans").arg(scans).arg(calScans));
}
