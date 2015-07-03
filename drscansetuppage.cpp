#include "drscansetuppage.h"
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
//    drSsw->setFtmFreq(ssw->ftmFreq());
//	drSsw->setAttn(ssw->attn());
//	drSsw->setDrPower(ssw->drPower());
//	drSsw->setPulseConfig(ssw->pulseConfig());

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

//	scan.setTargetShots(drSsw->shots());
//	scan.setFtFreq(drSsw->ftmFreq());
//	scan.setAttenuation(drSsw->attn());
//	scan.setDrFreq(drSsw->drFreq());
//	scan.setDrPower(drSsw->drPower());
//	scan.setPulseConfiguration(drSsw->pulseConfig());

	emit drScan(scan);
    emit fitter(a);

    return true;
}

void DrScanSetupPage::initializePage()
{
    drSsw->setDrFreq(field(QString("drStart")).toDouble());
    drSsw->setDrSynthBoxEnabled(false);
}
