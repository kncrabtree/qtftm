#ifndef CALSETUPPAGE_H
#define CALSETUPPAGE_H

#include <QWizardPage>
#include "batchwizard.h"

class CalSetupPage : public QWizardPage
{
	Q_OBJECT
public:
    explicit CalSetupPage(SingleScanWidget *ssw, BatchWizard::Page np, QWidget *parent = nullptr);
	int nextId() const;
	bool validatePage();

    const SingleScanWidget *scanWidget(){ return p_ssw; }
	
signals:
	void calScan(Scan s);
	
public slots:

private:
    SingleScanWidget *p_ssw;
    BatchWizard::Page d_nextPage;
	
};

#endif // CALSETUPPAGE_H
