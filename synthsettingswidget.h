#ifndef SYNTHSETTINGSWIDGET_H
#define SYNTHSETTINGSWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class SynthSettingsWidget;
}

class SynthSettingsWidget : public SettingsWidget
{
	Q_OBJECT
	
public:
	explicit SynthSettingsWidget(QWidget *parent = nullptr);
	~SynthSettingsWidget();
	
protected:
	Ui::SynthSettingsWidget *ui;
	bool d_bandInfoChanged;
	int d_lastBand;
    QString d_key;
    QString d_subKey;

public slots:
	void loadSettings();
	void saveSettings();

	void updateBandsComboBox();
	void loadBandInfo();
	void bandInfoChanged(){ d_bandInfoChanged = true; }
	void saveBandSettings(int band);

signals:
    void bandChanged();
    void switchPointChanged();

};

#endif // SYNTHSETTINGSWIDGET_H
