#ifndef SURVEYSCANSETUPPAGE_H
#define SURVEYSCANSETUPPAGE_H

#include <QWizardPage>
#include "batchwizard.h"

class SurveyScanSetupPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit SurveyScanSetupPage(SingleScanWidget *ssw, QWidget *parent = nullptr);
	int nextId() const;
	void initializePage();
	bool validatePage();

	const SingleScanWidget *scanWidget(){ return surveyScanSsw; }
	
signals:
	void surveyScan(Scan s);
	
public slots:
	void updateLabel();

private:
	SingleScanWidget *surveyScanSsw;
	QLabel *timeLabel;
	
};

#endif // SURVEYSCANSETUPPAGE_H
