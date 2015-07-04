#ifndef HVPOWERSUPPLY_H
#define HVPOWERSUPPLY_H

#include "hardwareobject.h"

class HvPowerSupply : public HardwareObject
{
	Q_OBJECT
public:
	explicit HvPowerSupply(QObject *parent = nullptr);
	~HvPowerSupply();

signals:
	void voltageUpdate(int);

public slots:
	int setVoltage(int v);
	int readVoltage();

protected:
	int d_hardwareMin;
	int d_hardwareMax;
	virtual void hwSetVoltage(int v) =0;
	virtual int hwReadVoltage() =0;
};

#ifdef QTFTM_HVPS
#if QTFTM_HVPS==1
#include "xxxx.h"
class XXXX;
typedef XXXX HvPowerSupplyHardware
#else
#include "virtualhvps.h"
class VirtualHVPS;
typedef VirtualHVPS HvPowerSupplyHardware;
#endif
#endif

#endif // HVPOWERSUPPLY_H
