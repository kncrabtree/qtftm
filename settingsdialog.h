#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "settingswidget.h"

class SettingsDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit SettingsDialog(SettingsWidget *w, QWidget *parent = nullptr);
	~SettingsDialog();
	
};

#endif // SETTINGSDIALOG_H
