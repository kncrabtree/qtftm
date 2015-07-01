#include "dr2dtemplatepage.h"
#include "batchwizard.h"

////NOT USED RIGHT NOW!
Dr2dTemplatePage::Dr2dTemplatePage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("2D DR Template"));
	setSubTitle(QString("Enter (or load) FTM parameters on the left, and DR parameters on the right. For each FTM frequency, a scan will be recorded at each DR frequency. Optionally, background scans can be taken between each DR measurement."));
}

int Dr2dTemplatePage::nextId() const
{
	return BatchWizard::Page_BatchProcessing;
}
