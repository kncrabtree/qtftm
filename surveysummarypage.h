#ifndef SURVEYSUMMARYPAGE_H
#define SURVEYSUMMARYPAGE_H

#include <QWizardPage>
#include <QLabel>
#include <QCheckBox>

#include "batchwizard.h"
#include "batchsurvey.h"

class SurveySummaryPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit SurveySummaryPage(QWidget *parent = nullptr);
	int nextId() const;
	void initializePage();
	bool validatePage();
	
signals:
	void batchSurvey(BatchManager*);


private:
	QLabel *label;
    QCheckBox *sleepCheckBox;
	
};

#endif // SURVEYSUMMARYPAGE_H
