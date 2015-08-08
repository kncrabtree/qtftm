#ifndef CATEGORYSCANSETUPPAGE_H
#define CATEGORYSCANSETUPPAGE_H

#include <QWizardPage>

class SingleScanWidget;
class BatchWidget;
class BatchManager;

class CategoryScanSetupPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit CategoryScanSetupPage(SingleScanWidget *ssw, QWidget *parent = nullptr);
	~CategoryScanSetupPage();

	// QWizardPage interface
	bool validatePage();
	bool isComplete() const;
	int nextId() const;
	void initializePage();

signals:
	void batchManager(BatchManager*);

private:
	BatchWidget *p_bw;
};

#endif // CATEGORYSCANSETUPPAGE_H
