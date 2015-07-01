#include "batchtemplatepage.h"
#include <QVBoxLayout>
#include <QRadioButton>
#include <QVariant>
#include "batchwizard.h"

////THIS IS NOT PRESENTLY USED!!!
BatchTemplatePage::BatchTemplatePage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Batch Template Selection"));
	setSubTitle(QString("Choose one of the templates below, or build your own by choosing the Custom option."));
	QVBoxLayout *vl = new QVBoxLayout(this);

	QRadioButton *toggle = new QRadioButton(QString("Toggle pulses"),this);
	toggle->setToolTip(QString("For a list of FTM cavity frequencies, turn on and off various pulse combinations"));
	vl->addWidget(toggle);

	QRadioButton *dr2d = new QRadioButton(QString("2D DR"),this);
	dr2d->setToolTip(QString("Cross-correlate a list of FTM frequencies with a list of DR frequencies"));
	vl->addWidget(dr2d);

	QRadioButton *custom = new QRadioButton(QString("Custom"),this);
	custom->setToolTip(QString("Configure scans individually"));
	vl->addWidget(custom);

	toggle->setChecked(true);

	setLayout(vl);

	registerField(QString("toggleTemplate"),toggle);
	registerField(QString("dr2dTemplate"),dr2d);
	registerField(QString("customTemplate"),custom);
}

int BatchTemplatePage::nextId() const
{
	if(field(QString("toggleTemplate")).toBool())
		return BatchWizard::Page_ToggleTemplate;

	if(field(QString("dr2dTemplate")).toBool())
		return BatchWizard::Page_Dr2dTemplate;

	if(field(QString("customTemplate")).toBool())
		return BatchWizard::Page_BatchProcessing;

	return -1;
}
