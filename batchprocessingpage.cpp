#include "batchprocessingpage.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include "batchwizard.h"

BatchProcessingPage::BatchProcessingPage(AutoFitWidget *afw, QWidget *parent) :
	QWizardPage(parent), d_type(QtFTM::Batch)
{
	setTitle(QString("Batch Processing Settings"));
	setSubTitle(QString("Enter the default settings to be used for processing batch scans."));

	QVBoxLayout *vl = new QVBoxLayout(this);

    batchAfw = new AutoFitWidget(afw->bufferGas().name,afw->delay(),afw->hpf(),afw->exp(),afw->zeroPad(),afw->isUseWindow(),afw->temperature(),this);
    vl->addWidget(batchAfw);

	setLayout(vl);
}

int BatchProcessingPage::nextId() const
{
	if(d_type == QtFTM::Batch)
		return BatchWizard::Page_BatchSetup;

    if(d_type == QtFTM::Categorize)
        return BatchWizard::Page_Categorize;

	if(d_type == QtFTM::DrCorrelation)
		return BatchWizard::Page_DrCorrSetup;

	return BatchWizard::Page_BatchSetup;
}

bool BatchProcessingPage::validatePage()
{
    emit fitter(batchAfw->toFitter());
	return true;
}

void BatchProcessingPage::setType(QtFTM::BatchType type)
{
	d_type = type;
}
