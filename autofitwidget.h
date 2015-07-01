#ifndef AUTOFITWIDGET_H
#define AUTOFITWIDGET_H

#include <QWidget>
#include <QList>
#include "analysis.h"
#include "lorentziandopplerlmsfitter.h"
#include "nofitter.h"

namespace Ui {
class AutoFitWidget;
}

class AutoFitWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AutoFitWidget(QString bgName = QString(), double d = 0.0, double h = 0.0, double e = 0.0, bool zp = false, double t = 293.15, QWidget *parent = nullptr);
	~AutoFitWidget();

	bool autoFitEnabled() const;
	int sourceConfig() const;
	FitResult::BufferGas bufferGas() const;
	double temperature() const;
	double delay() const;
	double hpf() const;
	double exp() const;
	bool removeDC() const;
	bool zeroPad() const;

	void setNoDisable();
	void disableProcessingOptions(bool disable = true);
	AbstractFitter *toFitter();

private:
	Ui::AutoFitWidget *ui;
	QList<FitResult::BufferGas> d_gases;

signals:
	void autoFitEnabledChanged(bool en);
};

#endif // AUTOFITWIDGET_H
