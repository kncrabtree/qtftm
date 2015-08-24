#include "batchwizard.h"
#include "startpage.h"
#include "surveysetuppage.h"
#include "surveycalsetuppage.h"
#include "surveyscansetuppage.h"
#include "surveysummarypage.h"
#include "drsetuppage.h"
#include "drscansetuppage.h"
#include "drsummarypage.h"
#include "batchtemplatepage.h"
#include "togglepulsespage.h"
#include "categorysetuppage.h"
#include "categoryscansetuppage.h"
#include "drcorrpage.h"
#include "batchsetuppage.h"
#include "batchprocessingpage.h"

BatchWizard::BatchWizard(SingleScanWidget *w, AutoFitWidget *a, QWidget *parent) :
	QWizard(parent), ssw(w), afw(a), bm(nullptr), ftr(nullptr)
{
	setWindowTitle(QString("Batch Acquisition Setup"));
	setDefaultProperty("QDoubleSpinBox","value","valueChanged");

	//make pages and connections, if necessary
	StartPage *sp = new StartPage(this);

    SurveySetupPage *sSetP = new SurveySetupPage(afw,this);
    connect(sSetP,&SurveySetupPage::fitter,this,&BatchWizard::setFitter);

	SurveyCalSetupPage *scp = new SurveyCalSetupPage(ssw,this);
	connect(scp,&SurveyCalSetupPage::calScan,this,&BatchWizard::setSurveyCal);

	SurveyScanSetupPage *ssp = new SurveyScanSetupPage(ssw,this);
	connect(ssp,&SurveyScanSetupPage::surveyScan,this,&BatchWizard::setSurveyScan);

	SurveySummaryPage *ssump = new SurveySummaryPage(this);
	connect(ssump,&SurveySummaryPage::batchSurvey,this,&BatchWizard::setBatchManager);

	DrScanSetupPage *dsp = new DrScanSetupPage(ssw,a,this);
	connect(dsp,&DrScanSetupPage::drScan,this,&BatchWizard::setDrScan);
	connect(dsp,&DrScanSetupPage::drScan,this,&BatchWizard::prepareDr);
	connect(dsp,&DrScanSetupPage::fitter,this,&BatchWizard::setFitter);

	dip = new DrIntSetupPage(this);
	connect(dip,&DrIntSetupPage::ranges,this,&BatchWizard::setDrRanges);

	DrSummaryPage *dsump = new DrSummaryPage(this);
	connect(dsump,&DrSummaryPage::batchDr,this,&BatchWizard::setBatchManager);

    BatchProcessingPage *bpp = new BatchProcessingPage(afw,this);
    connect(sp,&StartPage::typeSelected,bpp,&BatchProcessingPage::setType);
    connect(bpp,&BatchProcessingPage::fitter,this,&BatchWizard::setFitter);

	BatchSetupPage *bsp = new BatchSetupPage(ssw,this);
	connect(bsp,&BatchSetupPage::batchManager,this,&BatchWizard::setBatchManager);

	DrCorrPage *drcp = new DrCorrPage(ssw,this);
	connect(drcp,&DrCorrPage::batchManager,this,&BatchWizard::setBatchManager);

	CategoryScanSetupPage *cssp = new CategoryScanSetupPage(ssw,this);
	connect(cssp,&CategoryScanSetupPage::batchManager,this,&BatchWizard::setBatchManager);

	//insert pages into wizard
	setPage(Page_Start, sp);
    setPage(Page_SurveySetup, sSetP);
	setPage(Page_SurveyCalSetup, scp);
	setPage(Page_SurveyScanSetup, ssp);
	setPage(Page_SurveySummary, ssump);
	setPage(Page_DrSetup, new DrSetupPage(this));
	setPage(Page_DrScanSetup, dsp);
	setPage(Page_DrIntSetup, dip);
	setPage(Page_DrSummary, dsump);
//	setPage(Page_BatchTemplate, new BatchTemplatePage(this));
//	setPage(Page_ToggleTemplate, new TogglePulsesPage(this));
    setPage(Page_Categorize, new CategorySetupPage(this));
    setPage(Page_CategorizeScanSetup, cssp);
	setPage(Page_DrCorrSetup, drcp);
    setPage(Page_BatchProcessing, bpp);
	setPage(Page_BatchSetup, bsp);

	setGeometry(parent->width()/2-350,parent->height()/2-300,700,600);

}

BatchWizard::~BatchWizard()
{
	//delete the fitter if it has been allocated but not assigned to a batch Manager
	if(!bm)
	{
		if(ftr)
			delete ftr;
    }
}

void BatchWizard::setFitter(AbstractFitter *af)
{
    if(ftr)
    {
        delete ftr;
        ftr = nullptr;
    }

    ftr = af;
}

void BatchWizard::prepareDr(Scan s)
{
	Scan setup(s);
	//make sure the scan is set to dummy
	setup.setDummy();
	//turn off DR pulse; don't need it for now
    PulseGenConfig config = setup.pulseConfiguration();
    config.setDrEnabled(false);
	setup.setPulseConfiguration(config);

	emit setupDr(setup);
}

void BatchWizard::drPrepComplete()
{
	if(currentId() == Page_DrIntSetup) {
		connect(this,&BatchWizard::newFid,dip,&DrIntSetupPage::newFid,Qt::UniqueConnection);
	}
}
