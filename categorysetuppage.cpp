#include "categorysetuppage.h"
#include "ui_categorysetuppage.h"

#include <QMessageBox>

#include "voltagetestmodel.h"
#include "dipoletestmodel.h"
#include "batchwizard.h"
#include "batchcategorize.h"

CategorySetupPage::CategorySetupPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::CategorySetupPage)
{
    ui->setupUi(this);

    ui->voltagesView->setModel(new VoltageTestModel(this));
    ui->voltagesView->setItemDelegate(new VoltageTestDelegate(this));
    connect(ui->voltageAddButton,&QToolButton::clicked,[=](){ ui->voltagesView->model()->insertRow(-1); });
    connect(ui->voltageRemoveButton,&QToolButton::clicked,[=](){ui->voltagesView->model()->removeRow(ui->voltagesView->currentIndex().row()); });
    connect(ui->voltageTestBox,&QCheckBox::toggled,ui->voltagesBox,&QGroupBox::setEnabled);

    ui->dipolesView->setModel(new DipoleTestModel(this));
    ui->dipolesView->setItemDelegate(new DipoleTestDelegate(this));
    connect(ui->dipoleAddButton,&QToolButton::clicked,[=](){ ui->dipolesView->model()->insertRow(-1); });
    connect(ui->dipoleRemoveButton,&QToolButton::clicked,[=](){ui->dipolesView->model()->removeRow(ui->dipolesView->currentIndex().row()); });
    connect(ui->dipoleTestBox,&QCheckBox::toggled,ui->dipolesBox,&QGroupBox::setEnabled);

    connect(ui->dipoleTestBox,&QCheckBox::toggled,[=](bool en) {
	    if(!en)
		    ui->dipoleCategoryBox->setChecked(false);
	    ui->dipoleCategoryBox->setEnabled(en);
    });
    connect(ui->dcTestBox,&QCheckBox::toggled,[=](bool en) {
	    if(!en)
		    ui->dcCategoryBox->setChecked(false);
	    ui->dcCategoryBox->setEnabled(en);
    });
    connect(ui->voltageTestBox,&QCheckBox::toggled,[=](bool en) {
	    if(!en)
		    ui->voltageCategoryBox->setChecked(false);
	    ui->voltageCategoryBox->setEnabled(en);
    });
    connect(ui->magnetTestBox,&QCheckBox::toggled,[=](bool en) {
	    if(!en)
		    ui->magnetCategoryBox->setChecked(false);
	    ui->magnetCategoryBox->setEnabled(en);
    });

    registerField(QString("catDipole"),ui->dipoleCategoryBox);
    registerField(QString("catDc"),ui->dcCategoryBox);
    registerField(QString("catVoltage"),ui->voltageCategoryBox);
    registerField(QString("catMagnet"),ui->magnetCategoryBox);
    registerField(QString("freqWindow"),ui->frequencyWindowSpinBox);
}

CategorySetupPage::~CategorySetupPage()
{
    delete ui;
}


void CategorySetupPage::initializePage()
{

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("wizard"));

    ui->frequencyWindowSpinBox->setValue(s.value(QString("catFreqWindow"),100).toInt());

    s.endGroup();
}

bool CategorySetupPage::validatePage()
{
	int numTests = 0;
	QVariantList dip, dc, v, mag;

	//test order: dipole->DC on/off->DC Voltage->Magnet
	if(ui->dipoleTestBox->isChecked())
	{
		auto list = static_cast<DipoleTestModel*>(ui->dipolesView->model())->dipoleList();
		numTests += list.size();
		for(int i=0; i<list.size(); i++)
			dip.append(list.at(i));
	}

	//note: if dipoles already checked, we only need to do one more scan to test DC (either turn it on or off, depending on how it starts)
	//otherwise, this is the first test, so we need to check DC on vs off
    if(ui->dcTestBox->isChecked())
    {
	    dc << true << false;
	    if(numTests == 0)
		    numTests += 2;
	    else
		    numTests++;
    }

    if(ui->voltageTestBox->isChecked())
    {
	    auto list = static_cast<VoltageTestModel*>(ui->voltagesView->model())->voltageList();
	   numTests += list.size();
	   for(int i=0; i<list.size(); i++)
		   v.append(list.at(i));
    }

    //same as dc on/off test...
    if(ui->magnetTestBox->isChecked())
    {
	    mag << false << true;
	    if(numTests == 0)
		    numTests += 2;
	    else
		    numTests++;
    }

    if(numTests < 1)
    {
        QMessageBox::warning(this,QString("No tests selected."),QString("Cannot continue because no tests have been selected."));
        return false;
    }

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

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("wizard"));

    s.setValue(QString("catFreqWindow"),ui->frequencyWindowSpinBox->value());

    s.endGroup();

    emit catTests(testList);
    return true;
}

int CategorySetupPage::nextId() const
{
    return BatchWizard::Page_CategorizeScanSetup;
}
