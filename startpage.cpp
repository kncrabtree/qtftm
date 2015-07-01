#include "startpage.h"
#include <QVBoxLayout>
#include "batchwizard.h"
#include <QGroupBox>

StartPage::StartPage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Choose a batch type"));
	setSubTitle(QString("Select the type of batch operation you would like."));

	QVBoxLayout *vl = new QVBoxLayout(this);

	surveyButton = new QRadioButton(QString("Survey"),this);
	surveyButton->setToolTip(QString("A series of scans yielding continuous frequency coverage"));

	drButton = new QRadioButton(QString("DR Scan"),this);
	drButton->setToolTip(QString("Scans the double resonance synthesizer while sitting at a single FT frequency"));

	batchButton = new QRadioButton(QString("Batch"),this);
	batchButton->setToolTip(QString("An arbitrary sequence of scans"));

	surveyButton->setChecked(true);

	vl->addWidget(surveyButton);
	vl->addWidget(drButton);
	vl->addWidget(batchButton);

	QGroupBox *miscBox = new QGroupBox(QString("Miscellaneous Settings"),this);
	QVBoxLayout *mbl = new QVBoxLayout(miscBox);

	sleepCheckBox = new QCheckBox(QString("Sleep when batch is complete"),this);
	sleepCheckBox->setToolTip(QString("If checked, the instrument will be put into sleep mode when the acquisition is complete.\nThis will stop pulses from being generated, and will turn off the gas flow controllers."));
	sleepCheckBox->setChecked(false);
	mbl->addWidget(sleepCheckBox);

	miscBox->setLayout(mbl);
	vl->addWidget(miscBox);
	setLayout(vl);

#ifdef CONFIG_NODRSYNTH
    drButton->setEnabled(false);
#endif

}

int StartPage::nextId() const
{
	if(surveyButton->isChecked())
		return BatchWizard::Page_SurveySetup;

	if(drButton->isChecked())
		return BatchWizard::Page_DrSetup;

	if(batchButton->isChecked())
		return BatchWizard::Page_BatchProcessing;

	return -1;
}

bool StartPage::validatePage()
{
	emit sleepOnComplete(sleepCheckBox->isChecked());
	return true;
}
