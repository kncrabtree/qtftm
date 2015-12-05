#include "ftsynthsettingswidget.h"

#include <QMessageBox>

FtSynthSettingsWidget::FtSynthSettingsWidget(QWidget *parent) :
     SynthSettingsWidget(parent)
{
	d_title = QString("FTM Stynthesizer Settings");
	d_key = QString("ftmSynth");

	loadSettings();
    connect(this,&FtSynthSettingsWidget::switchPointChanged,this,&FtSynthSettingsWidget::warn);
}

void FtSynthSettingsWidget::warn()
{
    QMessageBox::warning(this,QString("Band Switch Point Changed"),QString("You have changed the band switch point for the FT synthesizer. Be sure to update your attentuation table appropriately."));
}
