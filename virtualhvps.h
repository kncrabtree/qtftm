#ifndef VIRTUALHVPS_H
#define VIRTUALHVPS_H

#include "hvpowersupply.h"

class VirtualHVPS : public HvPowerSupply
{
public:
	explicit VirtualHVPS(QObject *parent = nullptr);
	~VirtualHVPS();

	// HardwareObject interface
public slots:
	bool testConnection();
	void initialize();

	// HvPowerSupply interface
protected:
    bool hwSetVoltage(int v);
	int hwReadVoltage();

private:
	int d_currentVoltage;
};

#endif // VIRTUALHVPS_H
