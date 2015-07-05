#ifndef BATCHSETUPPAGE_H
#define BATCHSETUPPAGE_H

#include <QWizardPage>
#include "singlescanwidget.h"
#include "batchwidget.h"
#include "batch.h"

class BatchSetupPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit BatchSetupPage(SingleScanWidget *ssw, QWidget *parent = nullptr);
	bool isComplete() const;
	bool validatePage();
	
signals:
	void batchManager(BatchManager *bm);
	
public slots:

private:
	BatchWidget *bw;
	
};

#endif // BATCHSETUPPAGE_H
