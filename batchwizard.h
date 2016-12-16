#ifndef BATCHWIZARD_H
#define BATCHWIZARD_H

#include <QWizard>
#include "singlescanwidget.h"
#include "scan.h"
#include "batchmanager.h"
#include "autofitwidget.h"
#include "abstractfitter.h"
#include "batchcategorize.h"

class DrIntSetupPage;
class StartPage;

/*!
 \brief Wizard for configuring batch acquisitions

 The Batch Wizard class is designed to allow a user to walk through the process of setting up a batch acquisition.
 At time of writing, there are three types of batch modes that can be configured: Survey, DR Scan, and Batch.
 Each type has a corresponding BatchManager subclass, and the wizard is used to create and initialize it.
 Current instrument settings are passed in the constructor of the BatchWizard in the form of a configured SingleScanWidget, which can be duplicated and displayed as needed.

 As a subclass of QWizard, the BatchWizard consists of a series of customized QWizardPage objects whose order is determined by the choices the user makes.
 All pages need to have a corresponding value in the Page enum.
 When the wizard begins, the Page_Start (StartPage) is shown, which asks the user to choose a type of batch operation.
 After the selection, one of three paths is chosen: Survey, DR Scan, or Batch.

 As an example, here is the sequence of events if Survey is chosen
 In the survey mode, SurveySetupPage comes next, prompting the user for start, stop, and step frequencies as well as information about calibration scans.
 If a calibration line is requested the next page is the SurveyCalSetupPage, which allows the user to configure the settings for calibrations.
 Otherwise/afterward, the SurveyScanSetupPage allows the user to set conditions for the survey scans (except cavity frequency, which is calculated for each scan according to the survey parameters).
 Finally, the SurveySummaryPage is shown, which provides an overview of the requested settings and time estimated for completion.
 The process for DR Scans and Batch acquisitions are similar

 To add a new type of batch acquisition to the wizard, the first step is to construct a BatchManager subclass (see BatchManager class) that will execute the acquisition.
 Once the subclass is defined, then make a series of QWizardPage subclasses that allow the user to select the appropriate settings that your BatchManager subclass needs to know.
 At the end last page of the wizard, the QWizardPage's validatePage() function needs to create and configure the batch manager, and call BatchWizard::setBatchManager() to allow the mainwindow to pass it along to the batchThread.

 It is possible to configure hardware and grab sample data for display on a QWizardPage.
 For an example, see the DrIntSetupPage, and its associated signal/slot connections here in the BatchWizard.
 If you need to access data from an earler page on a later page (such as when creating the BatchManager object), either use the QWizardPage::registerField() and QWizardPage::field() functions, or store them as variables in the BatchWizard and access them through the QWizardPage::wizard() pointer, dynamically casting it as a BatchWizard.
 Both approaches are used in various pages, depending on the type of data being stored and whether it can be associated with a QWidget or not.

*/
class BatchWizard : public QWizard
{
	Q_OBJECT
public:
    explicit BatchWizard(SingleScanWidget *w, AutoFitWidget *a, double pressure, bool pEnabled, QList<QPair<double, bool> > flows, QWidget *parent = nullptr);
	~BatchWizard();

    enum Page { Page_Start, Page_SurveySetup, Page_SurveyCalSetup, Page_SurveyScanSetup, Page_SurveySummary,
                Page_DrSetup, Page_DrScanSetup, Page_DrIntSetup, Page_DrSummary, Page_BatchTemplate,
                Page_ToggleTemplate, Page_DrCorrSetup, Page_Categorize, Page_CategorizeScanSetup,
                Page_BatchProcessing, Page_BatchSetup, Page_AmdorConfig, Page_AmdorCalSetup, Page_AmdorSetup };
    Scan scanTemplate() const { return d_scanTemplate; }
    Scan calTemplate() const { return d_calTemplate; }
    BatchManager *batchManager() const { return p_bm; }
    AbstractFitter *fitter() const { return p_ftr; }
	QList<QPair<double,double> > ranges() const { return d_drRanges; }
	QList<BatchCategorize::CategoryTest> catTests() { return d_catTests; }
	
signals:
	void setupDr(Scan);
	void newFid(Fid);
	void resetPeakUp();
	void changeNumPeakupFids(int);
	
public slots:
    void setScanTemplate(Scan s){ d_scanTemplate = s; }
    void setCalTemplate(Scan s){ d_calTemplate = s; }
	void setDrRanges(const QList<QPair<double,double> > r) { d_drRanges = r; }
	void setCatTests(const QList<BatchCategorize::CategoryTest> l) { d_catTests = l; }
    void setBatchManager(BatchManager *b);
    void setFitter(AbstractFitter *af);
	void prepareDr(Scan s);
	void drPrepComplete();

private:
    SingleScanWidget *p_ssw;
    AutoFitWidget *p_afw;
    Scan d_scanTemplate;
    Scan d_calTemplate;
	QList<QPair<double,double> > d_drRanges;
	QList<BatchCategorize::CategoryTest> d_catTests;

    BatchManager *p_bm;
    AbstractFitter *p_ftr;
    DrIntSetupPage *p_dip;
    StartPage *p_start;
	
};

#endif // BATCHWIZARD_H
