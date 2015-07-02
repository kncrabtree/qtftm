#include "drsynthsettingswidget.h"

DrSynthSettingsWidget::DrSynthSettingsWidget(QWidget *parent) :
     SynthSettingsWidget(parent)
{
    d_title = QString("DR Synthesizer Settings");
	d_key = QString("drSynth");

	loadSettings();
}
