#include "singlescandialog.h"
#include "ui_singlescandialog.h"

SingleScanDialog::SingleScanDialog(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::SingleScanDialog)
{
	ui->setupUi(this);
}

SingleScanDialog::~SingleScanDialog()
{
	delete ui;
}

SingleScanWidget *SingleScanDialog::ssWidget()
{
	return ui->singleScanWidget;
}

void SingleScanDialog::insertAutoFitWidget(AutoFitWidget *w)
{
	if(!w)
		return;

	ui->horizontalLayout->insertWidget(1,w);
}
