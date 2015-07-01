#ifndef SURVEYCALSETUPPAGE_H
#define SURVEYCALSETUPPAGE_H

#include <QWizardPage>
#include "batchwizard.h"

class SurveyCalSetupPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit SurveyCalSetupPage(SingleScanWidget *ssw, QWidget *parent = nullptr);
	int nextId() const;
	bool validatePage();

	const SingleScanWidget *scanWidget(){ return surveyCalSsw; }
	
signals:
	void calScan(Scan s);
	
public slots:

private:
	SingleScanWidget *surveyCalSsw;
	
};

#endif // SURVEYCALSETUPPAGE_H
