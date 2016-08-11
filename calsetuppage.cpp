#include "calsetuppage.h"
#include <QVariant>
#include <QVBoxLayout>
#include "batchwizard.h"

CalSetupPage::CalSetupPage(SingleScanWidget *ssw, BatchWizard::Page np, QWidget *parent) :
     QWizardPage(parent), d_page(np)
{
	setTitle("Calibration Scan Setup");
	setSubTitle(QString("These settings will be used for the calibration scan."));

	//apply default settings
    p_ssw = new SingleScanWidget(this);
    p_ssw->setFromScan(ssw->toScan());

    if(np == BatchWizard::Page_SurveyCalSetup)
        registerField(QString("surveyCalShots"),p_ssw->shotsSpinBox());
    if(np == BatchWizard::Page_AmdorCalSetup)
        registerField(QString("amdorCalShots"),p_ssw->shotsSpinBox());
	QVBoxLayout *vl = new QVBoxLayout(this);
    vl->addWidget(p_ssw);

	setLayout(vl);

}

int CalSetupPage::nextId() const
{
    if(d_page == BatchWizard::Page_SurveyCalSetup)
        return BatchWizard::Page_SurveyScanSetup;
    else
        return BatchWizard::Page_AmdorSetup;
}

bool CalSetupPage::validatePage()
{
    Scan scan = p_ssw->toScan();

	emit calScan(scan);

	return true;
}
