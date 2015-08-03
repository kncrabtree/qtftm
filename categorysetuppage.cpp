#include "categorysetuppage.h"
#include "ui_categorysetuppage.h"

#include <QMessageBox>

#include "voltagetestmodel.h"
#include "dipoletestmodel.h"
#include "batchwizard.h"

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
    if(ui->dcTestBox->isChecked() || ui->magnetTestBox->isChecked())
        return true;

    int numTests = 0;
    if(ui->voltageTestBox->isChecked())
        numTests += static_cast<VoltageTestModel*>(ui->voltagesView->model())->voltageList().size();

    if(numTests < 1)
    {
        QMessageBox::warning(this,QString("No tests selected."),QString("Cannot continue because no tests have been selected."));
        return false;
    }

    return true;
}

int CategorySetupPage::nextId() const
{
    return BatchWizard::Page_BatchSetup;
}
