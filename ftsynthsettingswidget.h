#ifndef FTSYNTHSETTINGSWIDGET_H
#define FTSYNTHSETTINGSWIDGET_H

#include "synthsettingswidget.h"

class FtSynthSettingsWidget : public SynthSettingsWidget
{
	Q_OBJECT
public:
	explicit FtSynthSettingsWidget(QWidget *parent = nullptr);
	
signals:
	
public slots:
    void warn();
	
};

#endif // FTSYNTHSETTINGSWIDGET_H
