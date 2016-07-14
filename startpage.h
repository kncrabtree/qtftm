#ifndef STARTPAGE_H
#define STARTPAGE_H

#include <QWizardPage>
#include <QRadioButton>
#include <QCheckBox>

#include "datastructs.h"

class StartPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit StartPage(QWidget *parent = nullptr);
	int nextId() const;
	bool validatePage();

signals:
	void typeSelected(QtFTM::BatchType type);
		
public slots:

private:
	QRadioButton *surveyButton;
	QRadioButton *drButton;
	QRadioButton *batchButton;
    QRadioButton *categorizeButton;
	QRadioButton *drCorrButton;
    QRadioButton *amdorButton;
	
};

#endif // STARTPAGE_H
