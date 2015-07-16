#include "drcorrpage.h"
#include "batchwizard.h"

#include "batchwidget.h"
#include "drcorrelation.h"

DrCorrPage::DrCorrPage(SingleScanWidget *ssw, QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("DR Correlation"));
	setSubTitle(QString("Enter parameters for each FTM frequency. All frequencies will be cross-correlated by DR. Tuning and DR pulses will be set automatically."));

	QVBoxLayout *vl = new QVBoxLayout(this);
	bw = new BatchWidget(ssw,QtFTM::DrCorrelation,this);
	connect(bw,&BatchWidget::scansChanged,this,&QWizardPage::completeChanged);
	vl->addWidget(bw);

	setLayout(vl);
}

int DrCorrPage::nextId() const
{
	return -1;
}


bool DrCorrPage::validatePage()
{
	//there must be at least one scan in the list
	QList<QPair<Scan,bool> > scanList = bw->getList();
	if(scanList.isEmpty())
		return false;

    BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());
    AbstractFitter *ftr = wiz->fitter();

	//create batch object
    BatchManager *b = new DrCorrelation(scanList,ftr);
    b->setSleepWhenComplete(bw->sleep());

	emit batchManager(b);

	return true;
}

bool DrCorrPage::isComplete() const
{
	return !bw->isEmpty();
}
