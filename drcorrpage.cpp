#include "drcorrpage.h"

#include <QVBoxLayout>

#include "batchwizard.h"
#include "batchwidget.h"
#include "drcorrelation.h"

DrCorrPage::DrCorrPage(SingleScanWidget *ssw, QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("DR Correlation"));
	setSubTitle(QString("Enter parameters for each FTM frequency. All frequencies will be cross-correlated by DR. Tuning and DR pulses will be set automatically."));

	QVBoxLayout *vl = new QVBoxLayout(this);
    p_bw = new BatchWidget(ssw,QtFTM::DrCorrelation,this);
    connect(p_bw,&BatchWidget::scansChanged,this,&QWizardPage::completeChanged);
    vl->addWidget(p_bw);

	setLayout(vl);
}

int DrCorrPage::nextId() const
{
	return -1;
}

void DrCorrPage::initializePage()
{
    p_bw->updateLabel();
}


bool DrCorrPage::validatePage()
{
	//there must be at least one scan in the list
    QList<QPair<Scan,bool> > scanList = p_bw->getList();
	if(scanList.isEmpty())
		return false;

    BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());
    AbstractFitter *ftr = wiz->fitter();

	//create batch object
    BatchManager *b = new DrCorrelation(scanList,ftr);
    b->setSleepWhenComplete(p_bw->sleep());

	emit batchManager(b);

	return true;
}

bool DrCorrPage::isComplete() const
{
    return !p_bw->isEmpty();
}
