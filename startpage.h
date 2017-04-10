#ifndef STARTPAGE_H
#define STARTPAGE_H

#include <QWizardPage>
#include <QRadioButton>
#include <QCheckBox>
#include <QDoubleSpinBox>

#include "datastructs.h"
#include "batchmanager.h"

class StartPage : public QWizardPage
{
	Q_OBJECT
public:
    explicit StartPage(double pressure, bool pEnabled, QList<QPair<double, bool> > flows, QWidget *parent = nullptr);
	int nextId() const;
	bool validatePage();
    void applyLimits(BatchManager *bm);

    struct FlowLimitWidgets {
        QDoubleSpinBox *minBox;
        QDoubleSpinBox *valBox;
        QDoubleSpinBox *maxBox;
        QCheckBox *enabledBox;
    };

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

    QDoubleSpinBox *p_pressureMinBox;
    QDoubleSpinBox *p_pressureMaxBox;
    QCheckBox *p_pressureEnabledBox;
    QList<FlowLimitWidgets> d_flowLimits;
	
};

#endif // STARTPAGE_H
