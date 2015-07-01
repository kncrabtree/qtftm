#ifndef SURVEYSETUPPAGE_H
#define SURVEYSETUPPAGE_H

#include <QWizardPage>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include "autofitwidget.h"

class SurveySetupPage : public QWizardPage
{
	Q_OBJECT
public:
    explicit SurveySetupPage(AutoFitWidget *afw, QWidget *parent = nullptr);
	int nextId() const;
    bool validatePage();
    void initializePage();
	
signals:
    void fitter(AbstractFitter*);
	
public slots:
	void updateLabel();

private:
	QLabel *sumLabel;
	QDoubleSpinBox *startBox;
	QDoubleSpinBox *stopBox;
	QSpinBox *stepBox;
	QCheckBox *calCheckBox;
	QSpinBox *scansPerCalBox;

    AutoFitWidget *surAfw;
	
};

#endif // SURVEYSETUPPAGE_H
