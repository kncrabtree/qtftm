#ifndef SINGLESCANWIDGET_H
#define SINGLESCANWIDGET_H

#include <QWidget>
#include "scan.h"
#include <QSpinBox>

namespace Ui {
class SingleScanWidget;
}

class SingleScanWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit SingleScanWidget(QWidget *parent = nullptr);
	~SingleScanWidget();

	int shots() const;
	double ftmFreq() const;
	int attn() const;
	double drFreq() const;
	double drPower() const;
    int protectionTime() const;
    int scopeTime() const;
    double dipoleMoment() const;
    PulseGenConfig pulseConfig() const;

	void setFtmFreq(double d);
	void setAttn(int a);
	void setDrFreq(double d);
	void setDrPower(double d);
	void setPulseConfig(const PulseGenConfig pc);
    void setProtectionTime(int a);
    void setScopeTime(int a);
    void setMagnet(bool b);

    void enableSkipTune(bool enable = true);

    void setFromScan(const Scan s);
    Scan toScan() const;

    QSpinBox *shotsSpinBox();
    void setFtmSynthBoxEnabled(bool enabled);
    void setDrSynthBoxEnabled(bool enabled);
    void setShotsBoxEnabled(bool enabled);

public slots:
	void shotsChanged(int newShots);

private:
	Ui::SingleScanWidget *ui;


};

#endif // SINGLESCANWIDGET_H
