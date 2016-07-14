#ifndef DRCORRPAGE_H
#define DRCORRPAGE_H

#include <QWizardPage>

class BatchWidget;
class BatchManager;
class SingleScanWidget;

class DrCorrPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit DrCorrPage(SingleScanWidget *ssw, QWidget *parent = nullptr);
	int nextId() const;
	void initializePage();
	
signals:
	void batchManager(BatchManager*);
	
public slots:

private:
    BatchWidget *p_bw;
		
	// QWizardPage interface
public:
	bool validatePage();
	bool isComplete() const;
};

#endif // DRCORRPAGE_H
