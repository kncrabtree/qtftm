#include "surveycalsetuppage.h"
#include <QVariant>
#include <QVBoxLayout>
#include "batchwizard.h"

SurveyCalSetupPage::SurveyCalSetupPage(SingleScanWidget *ssw, QWidget *parent) :
     QWizardPage(parent)
{
	setTitle("Calibration Scan Setup");
	setSubTitle(QString("These settings will be used for the calibration scan."));

	//apply default settings
	surveyCalSsw = new SingleScanWidget(this);
    surveyCalSsw->setFromScan(ssw->toScan());
//	surveyCalSsw->setFtmFreq(ssw->ftmFreq());
//	surveyCalSsw->setAttn(ssw->attn());
//	surveyCalSsw->setDrFreq(ssw->drFreq());
//	surveyCalSsw->setDrPower(ssw->drPower());
//	surveyCalSsw->setPulseConfig(ssw->pulseConfig());

	registerField(QString("surveyCalShots"),surveyCalSsw->ui->ssShotsSpinBox);
	QVBoxLayout *vl = new QVBoxLayout(this);
	vl->addWidget(surveyCalSsw);

	setLayout(vl);

}

int SurveyCalSetupPage::nextId() const
{
	return BatchWizard::Page_SurveyScanSetup;
}

bool SurveyCalSetupPage::validatePage()
{
    Scan scan = surveyCalSsw->toScan();

	//get values from widget and make calibration scan
//	scan.setTargetShots(surveyCalSsw->shots());
//	scan.setFtFreq(surveyCalSsw->ftmFreq());
//	scan.setAttenuation(surveyCalSsw->attn());
//	scan.setDrFreq(surveyCalSsw->drFreq());
//	scan.setDrPower(surveyCalSsw->drPower());
//	scan.setPulseConfiguration(surveyCalSsw->pulseConfig());

	emit calScan(scan);

	return true;
}
