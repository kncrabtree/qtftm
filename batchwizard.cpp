#include "batchwizard.h"
#include "startpage.h"
#include "surveysetuppage.h"
#include "calsetuppage.h"
#include "surveyscansetuppage.h"
#include "surveysummarypage.h"
#include "drsetuppage.h"
#include "drintsetuppage.h"
#include "drscansetuppage.h"
#include "drsummarypage.h"
#include "batchtemplatepage.h"
#include "togglepulsespage.h"
#include "categorysetuppage.h"
#include "categoryscansetuppage.h"
#include "drcorrpage.h"
#include "batchsetuppage.h"
#include "batchprocessingpage.h"
#include "amdorconfigpage.h"
#include "amdorsetuppage.h"

BatchWizard::BatchWizard(SingleScanWidget *w, AutoFitWidget *a, QWidget *parent) :
    QWizard(parent), p_ssw(w), p_afw(a), p_bm(nullptr), p_ftr(nullptr)
{
	setWindowTitle(QString("Batch Acquisition Setup"));
	setDefaultProperty("QDoubleSpinBox","value","valueChanged");

	//make pages and connections, if necessary
	StartPage *sp = new StartPage(this);

    SurveySetupPage *sSetP = new SurveySetupPage(p_afw,this);
    connect(sSetP,&SurveySetupPage::fitter,this,&BatchWizard::setFitter);

    CalSetupPage *scp = new CalSetupPage(p_ssw,Page_SurveyScanSetup, this);
    connect(scp,&CalSetupPage::calScan,this,&BatchWizard::setCalTemplate);

    SurveyScanSetupPage *ssp = new SurveyScanSetupPage(p_ssw,this);
    connect(ssp,&SurveyScanSetupPage::surveyScan,this,&BatchWizard::setScanTemplate);

	SurveySummaryPage *ssump = new SurveySummaryPage(this);
	connect(ssump,&SurveySummaryPage::batchSurvey,this,&BatchWizard::setBatchManager);

    DrScanSetupPage *dsp = new DrScanSetupPage(p_ssw,a,this);
    connect(dsp,&DrScanSetupPage::drScan,this,&BatchWizard::setScanTemplate);
	connect(dsp,&DrScanSetupPage::drScan,this,&BatchWizard::prepareDr);
	connect(dsp,&DrScanSetupPage::fitter,this,&BatchWizard::setFitter);

    p_dip = new DrIntSetupPage(this);
    connect(p_dip,&DrIntSetupPage::ranges,this,&BatchWizard::setDrRanges);

	DrSummaryPage *dsump = new DrSummaryPage(this);
	connect(dsump,&DrSummaryPage::batchDr,this,&BatchWizard::setBatchManager);

    BatchProcessingPage *bpp = new BatchProcessingPage(p_afw,this);
    connect(sp,&StartPage::typeSelected,bpp,&BatchProcessingPage::setType);
    connect(bpp,&BatchProcessingPage::fitter,this,&BatchWizard::setFitter);

    BatchSetupPage *bsp = new BatchSetupPage(p_ssw,this);
	connect(bsp,&BatchSetupPage::batchManager,this,&BatchWizard::setBatchManager);

    DrCorrPage *drcp = new DrCorrPage(p_ssw,this);
	connect(drcp,&DrCorrPage::batchManager,this,&BatchWizard::setBatchManager);

	CategorySetupPage *csp = new CategorySetupPage(this);
	connect(csp,&CategorySetupPage::catTests,this,&BatchWizard::setCatTests);

    CategoryScanSetupPage *cssp = new CategoryScanSetupPage(p_ssw,this);
	connect(cssp,&CategoryScanSetupPage::batchManager,this,&BatchWizard::setBatchManager);

    AmdorConfigPage *acp = new AmdorConfigPage(p_afw,this);
    connect(acp,&AmdorConfigPage::fitter,this,&BatchWizard::setFitter);

    CalSetupPage *acsp = new CalSetupPage(p_ssw,Page_AmdorConfig,this);
    connect(acsp,&CalSetupPage::calScan,this,&BatchWizard::setCalTemplate);

    AmdorSetupPage *asp = new AmdorSetupPage(p_ssw,this);
    connect(asp,&AmdorSetupPage::batchManager,this,&BatchWizard::setBatchManager);

	//insert pages into wizard
	setPage(Page_Start, sp);
    setPage(Page_SurveySetup, sSetP);
	setPage(Page_SurveyCalSetup, scp);
	setPage(Page_SurveyScanSetup, ssp);
	setPage(Page_SurveySummary, ssump);
	setPage(Page_DrSetup, new DrSetupPage(this));
	setPage(Page_DrScanSetup, dsp);
    setPage(Page_DrIntSetup, p_dip);
	setPage(Page_DrSummary, dsump);
//	setPage(Page_BatchTemplate, new BatchTemplatePage(this));
//	setPage(Page_ToggleTemplate, new TogglePulsesPage(this));
    setPage(Page_Categorize, csp);
    setPage(Page_CategorizeScanSetup, cssp);
	setPage(Page_DrCorrSetup, drcp);
    setPage(Page_BatchProcessing, bpp);
	setPage(Page_BatchSetup, bsp);
    setPage(Page_AmdorConfig,acp);
    setPage(Page_AmdorCalSetup,acsp);

	setGeometry(parent->width()/2-350,parent->height()/2-300,700,600);

}

BatchWizard::~BatchWizard()
{
	//delete the fitter if it has been allocated but not assigned to a batch Manager
    if(!p_bm)
	{
        if(p_ftr)
            delete p_ftr;
    }
}

void BatchWizard::setFitter(AbstractFitter *af)
{
    if(p_ftr)
    {
        delete p_ftr;
        p_ftr = nullptr;
    }

    p_ftr = af;
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
        connect(this,&BatchWizard::newFid,p_dip,&DrIntSetupPage::newFid,Qt::UniqueConnection);
	}
}
