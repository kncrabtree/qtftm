#ifndef DRSETUPPAGE_H
#define DRSETUPPAGE_H

#include <QWizardPage>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QCheckBox>

class DrSetupPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit DrSetupPage(QWidget *parent = nullptr);
	int nextId() const;
    bool validatePage();
    void initializePage();
	
signals:
	
public slots:
	void updateLabel();

private:
	QLabel *label;
	QDoubleSpinBox *startBox;
	QDoubleSpinBox *stopBox;
	QSpinBox *stepBox;
    QSpinBox *scansBetweenTuningVoltageReadings;
	QCheckBox *calCheckBox;

	
};

#endif // DRSETUPPAGE_H
