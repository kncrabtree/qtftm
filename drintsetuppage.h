#ifndef DRINTSETUPPAGE_H
#define DRINTSETUPPAGE_H

#include <QWizardPage>
#include "analysiswidget.h"
#include <QPushButton>
#include <QSpinBox>

class DrIntSetupPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit DrIntSetupPage(QWidget *parent = nullptr);
	int nextId() const;
	void initializePage();
	bool validatePage();
	void cleanupPage();
	bool isComplete() const;
	
signals:
	void newFid(Fid);
	void ranges(QList<QPair<double,double> >);
	
public slots:
	void updateLabel();

private:
	AnalysisWidget *aw;
	QSpinBox *shotsSpinBox;
	QPushButton *resetButton;
	QLabel *shotsLabel;
	QLabel *timeLabel;
	
};

#endif // DRINTSETUPPAGE_H
