#ifndef DRSCANSETUPPAGE_H
#define DRSCANSETUPPAGE_H

#include <QWizardPage>
#include "singlescanwidget.h"
#include "scan.h"
#include "autofitwidget.h"

class DrScanSetupPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit DrScanSetupPage(SingleScanWidget *ssw, AutoFitWidget *aw, QWidget *parent = nullptr);
	int nextId() const;
	bool validatePage();
    void initializePage();
	
signals:
	void drScan(Scan);
	void fitter(AbstractFitter*);
	
public slots:

private:
	SingleScanWidget *drSsw;
	AutoFitWidget *drAfw;
	
};

#endif // DRSCANSETUPPAGE_H
