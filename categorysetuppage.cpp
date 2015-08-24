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

    registerField(QString("catDipole"),ui->dipoleCategoryBox);
    registerField(QString("catDc"),ui->dcCategoryBox);
    registerField(QString("catVoltage"),ui->voltageCategoryBox);
    registerField(QString("catMagnet"),ui->magnetCategoryBox);
}

CategorySetupPage::~CategorySetupPage()
{
    delete ui;
}


void CategorySetupPage::initializePage()
{
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

    setField(QString("dipoleTestList"),dip);
    setField(QString("dcTestList"),dc);
    setField(QString("votlageTestList"),v);
    setField(QString("magnetTestList"),mag);
    setField(QString("numCatTests"),numTests);
    return true;
}

int CategorySetupPage::nextId() const
{
    return BatchWizard::Page_CategorizeScanSetup;
}
