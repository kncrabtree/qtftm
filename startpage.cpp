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


	miscBox->setLayout(mbl);
	vl->addWidget(miscBox);
	setLayout(vl);

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
	return true;
}
