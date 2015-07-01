#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QSettings>
#include <QApplication>

class SettingsWidget : public QWidget
{
	Q_OBJECT
public:
	explicit SettingsWidget(QWidget *parent = nullptr);
	QString title(){ return d_title; }
	
signals:
	void somethingChanged(bool b=true);
	
public slots:
	virtual void loadSettings() =0;
	virtual void saveSettings() =0;

protected:
	QString d_title;
	
};

#endif // SETTINGSWIDGET_H
