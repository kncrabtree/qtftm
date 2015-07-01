#include "surveysetuppage.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <math.h>
#include "batchwizard.h"
#include <QSettings>
#include <QApplication>

SurveySetupPage::SurveySetupPage(AutoFitWidget *afw, QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Enter survey parameters"));
	setSubTitle(QString("Fill in the survey parameters. You will be able to configure the details of the survey and calibration scans (if applicable) on the following pages."));

    surAfw = new AutoFitWidget(afw->bufferGas().name,afw->delay(),afw->hpf(),afw->exp(),afw->zeroPad(),afw->temperature());

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	double synthMin = s.value(QString("ftmSynth/min"),5000.0).toDouble();
	double synthMax = s.value(QString("ftmSynth/max"),43000.0).toDouble();

	QVBoxLayout *vl = new QVBoxLayout(this);

	QGroupBox *paramsBox = new QGroupBox(QString("Parameters"),this);
	QFormLayout *pBoxLayout = new QFormLayout(paramsBox);

	startBox = new QDoubleSpinBox(this);
	startBox->setObjectName(QString("surveyStartBox"));
	startBox->setDecimals(3);
	startBox->setMinimum(synthMin);
	startBox->setMaximum(synthMax);
	startBox->setSingleStep(100.0);
	startBox->setSuffix(QString(" MHz"));
	startBox->setValue(10000.0);
	startBox->setToolTip(QString("Cavity frequency at which the survey will begin"));
	startBox->setKeyboardTracking(false);

	stopBox = new QDoubleSpinBox(this);
	stopBox->setObjectName(QString("surveyStopBox"));
	stopBox->setDecimals(3);
	stopBox->setMinimum(synthMin);
	stopBox->setMaximum(synthMax);
	stopBox->setSingleStep(100.0);
	stopBox->setSuffix(QString(" MHz"));
	stopBox->setValue(11000.0);
	stopBox->setToolTip(QString("Cavity frequency at which the survey will end.\nThe final scan will be at this cavity frequency, or at the next step beyond (if the stop isn't an integer number of steps from the start)"));
	stopBox->setKeyboardTracking(false);

	double offset = s.value(QString("ftSynth/probeOffset"),0.400).toDouble();

	stepBox = new QSpinBox(this);
	stepBox->setObjectName(QString("surveyStepBox"));
	stepBox->setMinimum(10);
	stepBox->setMaximum(100000);
	stepBox->setSingleStep(100);
	stepBox->setSuffix(QString(" kHz"));
	stepBox->setValue((int)(offset*1000.0));
	stepBox->setToolTip(QString("Absolute value of step size (sign is determined from start and stop frequencies).\nIf the step size is larger than twice the offset between cavity and probe frequency (default: %1 kHz), there will be gaps").arg((int)(offset*1000.0)));
	stepBox->setKeyboardTracking(false);

	calCheckBox = new QCheckBox(this);
	calCheckBox->setChecked(true);
	calCheckBox->setToolTip(QString("When checked, calibration scans will be taken throughout the survey"));

	scansPerCalBox = new QSpinBox(this);
	scansPerCalBox->setObjectName(QString("surveyScansPerCalBox"));
	scansPerCalBox->setMinimum(0);
	scansPerCalBox->setMaximum(100000);
	scansPerCalBox->setSingleStep(25);
	scansPerCalBox->setValue(50);
	scansPerCalBox->setToolTip(QString("Number of survey steps between calibration scans.\nIf this is set to 0 or a number greater than the number of steps, calibration scans will be taken just at the beginning and end of the survey"));
	scansPerCalBox->setKeyboardTracking(false);

	//update label if anything changes
	auto doubleVc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
	auto intVc = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);

	connect(calCheckBox,&QAbstractButton::toggled,scansPerCalBox,&QWidget::setEnabled);
	connect(startBox,doubleVc,this,&SurveySetupPage::updateLabel);
	connect(stopBox,doubleVc,this,&SurveySetupPage::updateLabel);
	connect(stepBox,intVc,this,&SurveySetupPage::updateLabel);
	connect(calCheckBox,&QAbstractButton::toggled,this,&SurveySetupPage::updateLabel);
	connect(scansPerCalBox,intVc,this,&SurveySetupPage::updateLabel);

	pBoxLayout->addRow(QString("Start"),startBox);
	pBoxLayout->addRow(QString("Stop"),stopBox);
	pBoxLayout->addRow(QString("Step size"),stepBox);
	pBoxLayout->addRow(QString("Calibration?"),calCheckBox);
	pBoxLayout->addRow(QString("Scans per cal"),scansPerCalBox);

	paramsBox->setLayout(pBoxLayout);

	vl->addWidget(paramsBox);


	//calculate number of scans
	int surveyScans = (int)ceil(fabs(stopBox->value()-startBox->value())/(double)stepBox->value()*1000.0)+1.0;
	int calScans;
	if(calCheckBox->isChecked())
	{
		if(scansPerCalBox->value()>0)
			calScans = (int)ceil( (double)surveyScans/(double)scansPerCalBox->value() ) + 1;
		else
			calScans = 2;
	}
	else
		calScans = 0;

	sumLabel = new QLabel(QString("%1 Survey scans, %2 Calibration scans").arg(surveyScans).arg(calScans));

	vl->addWidget(sumLabel);
    vl->addWidget(surAfw);

	setLayout(vl);

	//register fields for later use
	registerField(QString("surveyStart"),startBox);
	registerField(QString("surveyStop"),stopBox);
	registerField(QString("surveyStep"),stepBox);
	registerField(QString("surveyCal"),calCheckBox);
	registerField(QString("surveyScansPerCal"),scansPerCalBox);
}

int SurveySetupPage::nextId() const
{
	if(calCheckBox->isChecked())
		return BatchWizard::Page_SurveyCalSetup;
	else
        return BatchWizard::Page_SurveyScanSetup;
}

bool SurveySetupPage::validatePage()
{
    emit fitter(surAfw->toFitter());
    return true;
}

void SurveySetupPage::updateLabel()
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
	int surveyScans = (int)ceil(fabs(stopBox->value()-startBox->value())/(double)stepBox->value()*1000.0)+1.0;
	int calScans;
	if(calCheckBox->isChecked())
	{
		if(scansPerCalBox->value()>0)
			calScans = (int)ceil( (double)surveyScans/(double)scansPerCalBox->value() ) + 1;
		else
			calScans = 2;
	}
	else
		calScans = 0;

	sumLabel->setText(QString("%1 Survey scans, %2 Calibration scans").arg(surveyScans).arg(calScans));
}
