#include "startpage.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

#include "batchwizard.h"
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

    categorizeButton = new QRadioButton(QString("Categorize"),this);
    categorizeButton->setToolTip(QString("Perform a series of tests on a list of lines."));

	drCorrButton = new QRadioButton(QString("DR Correlation"),this);
	drCorrButton->setToolTip(QString("A list of FT scans that will by systematically cross-correlated by double resonance."));

	surveyButton->setChecked(true);

	vl->addWidget(surveyButton);
	vl->addWidget(drButton);
	vl->addWidget(batchButton);
    vl->addWidget(categorizeButton);
	vl->addWidget(drCorrButton);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(QString("ftmSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	double ftMin = s.value(QString("min"),5000.0).toDouble();
	double ftMax = s.value(QString("max"),26000.0).toDouble();
	s.endGroup();
	s.endGroup();

	s.beginGroup(QString("drSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	double drMin = s.value(QString("min"),1000.0).toDouble();
	double drMax = s.value(QString("max"),26000.0).toDouble();
	s.endGroup();
	s.endGroup();

	double min = qMax(ftMin,drMin);
	double max = qMin(ftMax,drMax);

	if(max < min)
	{
		drCorrButton->setEnabled(false);
		QLabel *lbl = new QLabel(QString("DR correlation is disabled because the DR synthesizer cannot access the range of the FT. Reconfigure the DR synthesizer to match the FT range and try again."));
		lbl->setWordWrap(true);
		vl->addWidget(lbl);
	}

//	QGroupBox *miscBox = new QGroupBox(QString("Miscellaneous Settings"),this);
//	QVBoxLayout *mbl = new QVBoxLayout(miscBox);
//	miscBox->setLayout(mbl);
//	vl->addWidget(miscBox);
//
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

    if(categorizeButton->isChecked())
        return BatchWizard::Page_BatchProcessing;

	if(drCorrButton->isChecked())
		return BatchWizard::Page_BatchProcessing;

	return -1;
}

bool StartPage::validatePage()
{
	if(surveyButton->isChecked())
		emit typeSelected(QtFTM::Survey);
	else if(drButton->isChecked())
		emit typeSelected(QtFTM::DrScan);
	else if(batchButton->isChecked())
		emit typeSelected(QtFTM::Batch);
    else if(categorizeButton->isChecked())
        emit typeSelected(QtFTM::Categorize);
	else if(drCorrButton->isChecked())
		emit typeSelected(QtFTM::DrCorrelation);

	return true;
}
