#ifndef DRSUMMARYPAGE_H
#define DRSUMMARYPAGE_H

#include <QWizardPage>
#include <QLabel>
#include "batchdr.h"
#include "batchwizard.h"

class DrSummaryPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit DrSummaryPage(QWidget *parent = nullptr);
	int nextId() const;
	void initializePage();
	bool validatePage();
	
signals:
	void batchDr(BatchManager*);
    void sleepWhenComplete(bool);

private:
	QLabel *label;
    QCheckBox *sleepCheckBox;
	
};

#endif // DRSUMMARYPAGE_H
