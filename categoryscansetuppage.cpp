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
	p_bw = new BatchWidget(ssw,QtFTM::Batch,this);
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

    QVariantList dip = field(QString("dipoleTestList")).toList();
    QVariantList dc = field(QString("dcTestList")).toList();
    QVariantList v = field(QString("voltageTestList")).toList();
    QVariantList mag = field(QString("magnetTestList")).toList();

    QList<BatchCategorize::CategoryTest> testList;

    BatchCategorize::CategoryTest dipTest;
    dipTest.key = QString("u");
    dipTest.name = QString("Dipole test");
    dipTest.categorize = field(QString("catDipole")).toBool();
    dipTest.valueList = dip;
    testList.append(dipTest);

    BatchCategorize::CategoryTest dcTest;
    dcTest.key = QString("dc");
    dcTest.name = QString("Discharge test");
    dcTest.categorize = field(QString("catDc")).toBool();
    dcTest.valueList = dc;
    testList.append(dcTest);

    BatchCategorize::CategoryTest voltageTest;
    voltageTest.key = QString("v");
    voltageTest.name = QString("Voltage test");
    voltageTest.categorize = field(QString("catVoltage")).toBool();
    voltageTest.valueList = v;
    testList.append(voltageTest);

    BatchCategorize::CategoryTest magnetTest;
    magnetTest.key = QString("m");
    magnetTest.name = QString("Magnet test");
    magnetTest.categorize = field(QString("catMagnet")).toBool();
    magnetTest.valueList = mag;
    testList.append(magnetTest);

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
	int tests = field(QString("catNumTests")).toInt();
	if(tests > 0)
		p_bw->setNumTests(tests);
}
