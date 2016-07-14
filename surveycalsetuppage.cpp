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

	registerField(QString("surveyCalShots"),surveyCalSsw->shotsSpinBox());
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

	emit calScan(scan);

	return true;
}
