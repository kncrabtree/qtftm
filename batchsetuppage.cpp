#include "batchsetuppage.h"

#include <QVBoxLayout>

#include "batchwizard.h"

BatchSetupPage::BatchSetupPage(SingleScanWidget *ssw, QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Batch Setup"));
	setSubTitle(QString("Configure your batch scan by adding individual scans to the table."));

	QVBoxLayout *vl = new QVBoxLayout(this);
	bw = new BatchWidget(ssw,QtFTM::Batch,this);
	connect(bw,&BatchWidget::scansChanged,this,&QWizardPage::completeChanged);
	vl->addWidget(bw);

	setLayout(vl);

}

bool BatchSetupPage::isComplete() const
{
	return !bw->isEmpty();
}

bool BatchSetupPage::validatePage()
{
	//there must be at least one scan in the list
	QList<QPair<Scan,bool> > scanList = bw->getList();
	if(scanList.isEmpty())
		return false;

    BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());
    AbstractFitter *ftr = wiz->fitter();

	//create batch object
    Batch *b = new Batch(scanList,ftr);
    b->setSleepWhenComplete(bw->sleep());

	emit batchManager(b);

    return true;
}

void BatchSetupPage::initializePage()
{
	bw->updateLabel();
}
