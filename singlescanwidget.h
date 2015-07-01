#ifndef SINGLESCANWIDGET_H
#define SINGLESCANWIDGET_H

#include <QWidget>
#include "pulsegenerator.h"
#include "ui_singlescanwidget.h"
#include "scan.h"

namespace Ui {
class SingleScanWidget;
}

class SingleScanWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit SingleScanWidget(QWidget *parent = nullptr);
	~SingleScanWidget();
	Ui::SingleScanWidget *ui;

	int shots() const;
	double ftmFreq() const;
	int attn() const;
	double drFreq() const;
	double drPower() const;
    int protectionTime() const;
    int scopeTime() const;
    double dipoleMoment() const;
    QList<PulseGenerator::PulseChannelConfiguration> pulseConfig() const;

	void setFtmFreq(double d);
	void setAttn(int a);
	void setDrFreq(double d);
	void setDrPower(double d);
	void setPulseConfig(const QList<PulseGenerator::PulseChannelConfiguration> pc);
    void setProtectionTime(int a);
    void setScopeTime(int a);
    void setMagnet(bool b);

    void enableSkipTune(bool enable = true);

    void setFromScan(const Scan s);
    Scan toScan() const;

public slots:
	void toggleOnOffButtonText(bool on);
	void shotsChanged(int newShots);


private:
	QList<PulseGenerator::PulseChannelConfiguration> pConfig;

};

#endif // SINGLESCANWIDGET_H
