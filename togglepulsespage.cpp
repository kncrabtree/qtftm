#include "togglepulsespage.h"
#include "batchwizard.h"

//NOT USED RIGHT NOW!!!
TogglePulsesPage::TogglePulsesPage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Toggle Pulses Template"));
	setSubTitle(QString("Enter (or load) a list of frequencies and scan settings on the left, and choose which pulses to toggle on the right."));
}

int TogglePulsesPage::nextId() const
{
	return BatchWizard::Page_BatchProcessing;
}
