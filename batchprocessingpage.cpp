#include "batchprocessingpage.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include "batchwizard.h"

BatchProcessingPage::BatchProcessingPage(AutoFitWidget *afw, QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Batch Processing Settings"));
	setSubTitle(QString("Enter the default settings to be used for processing batch scans."));

	QVBoxLayout *vl = new QVBoxLayout(this);

    batchAfw = new AutoFitWidget(afw->bufferGas().name,afw->delay(),afw->hpf(),afw->exp(),afw->zeroPad(),afw->temperature(),this);
    vl->addWidget(batchAfw);

	setLayout(vl);
}

int BatchProcessingPage::nextId() const
{
    return BatchWizard::Page_BatchSetup;
}

bool BatchProcessingPage::validatePage()
{
    emit fitter(batchAfw->toFitter());
    return true;
}
