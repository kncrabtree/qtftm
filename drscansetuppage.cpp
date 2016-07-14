#include "drscansetuppage.h"

#include <QHBoxLayout>
#include <QCheckBox>

#include "batchwizard.h"

DrScanSetupPage::DrScanSetupPage(SingleScanWidget *ssw, AutoFitWidget *aw, QWidget *parent) :
	QWizardPage(parent)
{
	setTitle("DR Scan Setup");
	setSubTitle(QString("These settings will be used for the DR scan. The DR pulse and frequency will be set automatically. You will choose the number of shots and the scan processing settings (delay, high pass filter, etc) on the next page. If you enable autofitting, the baseline of the FT will be subtracted prior to integration."));

	//set default values
	drSsw = new SingleScanWidget(this);
    drSsw->setFromScan(ssw->toScan());
    drSsw->setShotsBoxEnabled(false);
    drSsw->enableSkipTune(true);

    drAfw = new AutoFitWidget(aw->bufferGas().name,aw->delay(),aw->hpf(),aw->exp(),aw->zeroPad(),aw->temperature(),this);
    drAfw->disableProcessingOptions();

	QHBoxLayout *vl = new QHBoxLayout(this);
	vl->addWidget(drSsw);
	vl->addWidget(drAfw);

	QCheckBox *hiddenCb = new QCheckBox(this);
	hiddenCb->setVisible(false);
	hiddenCb->setChecked(drAfw->autoFitEnabled());
	connect(drAfw,&AutoFitWidget::autoFitEnabledChanged,hiddenCb,&QAbstractButton::setChecked);

	registerField(QString("drAutoFit"),hiddenCb);

	setLayout(vl);
}

int DrScanSetupPage::nextId() const
{
	return BatchWizard::Page_DrIntSetup;
}

bool DrScanSetupPage::validatePage()
{
    Scan scan = drSsw->toScan();
    AbstractFitter *a = drAfw->toFitter();

	emit drScan(scan);
    emit fitter(a);

    return true;
}

void DrScanSetupPage::initializePage()
{
    drSsw->setDrFreq(field(QString("drStart")).toDouble());
    drSsw->setDrSynthBoxEnabled(false);
}
