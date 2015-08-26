#include "categoryscansetuppage.h"

#include <QVBoxLayout>

#include "batchwidget.h"
#include "batchwizard.h"
#include "batchcategorize.h"

CategoryScanSetupPage::CategoryScanSetupPage(SingleScanWidget *ssw, QWidget *parent) :
	QWizardPage(parent)
{
	setTitle(QString("Category Scan Setup"));
	setSubTitle(QString("Configure the list of frequencies to be categorized. For each scan, the settings you enter will be used until/unless a test overrides them."));

	QVBoxLayout *vl = new QVBoxLayout(this);
	p_bw = new BatchWidget(ssw,QtFTM::Categorize,this);
	connect(p_bw,&BatchWidget::scansChanged,this,&QWizardPage::completeChanged);
	vl->addWidget(p_bw);

	setLayout(vl);
}

CategoryScanSetupPage::~CategoryScanSetupPage()
{

}



bool CategoryScanSetupPage::validatePage()
{
	//there must be at least one scan in the list
	QList<QPair<Scan,bool> > scanList = p_bw->getList();
	if(scanList.isEmpty())
		return false;

    BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());
    AbstractFitter *ftr = wiz->fitter();

    auto testList = wiz->catTests();

	//create batch object
    BatchCategorize *b = new BatchCategorize(scanList,testList,ftr);
    b->setSleepWhenComplete(p_bw->sleep());

	emit batchManager(b);

	return true;
}

bool CategoryScanSetupPage::isComplete() const
{
	return !p_bw->isEmpty();
}

int CategoryScanSetupPage::nextId() const
{
	return -1;
}

void CategoryScanSetupPage::initializePage()
{
    BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());

    auto testList = wiz->catTests();
    int tests = 0;
    for(int i=0; i<testList.size(); i++)
        tests += testList.at(i).valueList.size();
	if(tests > 0)
        p_bw->setNumTests(tests+1);
}
