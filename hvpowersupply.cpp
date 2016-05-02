#include "hvpowersupply.h"

HvPowerSupply::HvPowerSupply(QObject *parent) :
	HardwareObject(parent)
{
	d_key = QString("hvps");
	d_isCritical = false;
}

HvPowerSupply::~HvPowerSupply()
{

}

int HvPowerSupply::setVoltage(int v)
{
	if(v < d_hardwareMin || v > d_hardwareMax)
    {
		emit logMessage(QString("Requested voltage (%1) is outside the hardware range (%2 - %3). Voltage was not set."),
					 QtFTM::LogWarning);
        return readVoltage();
    }
	else
    {
        if(hwSetVoltage(v))
            return readVoltage();
        else
            return -1;
    }

    //not reached
    return -1;

}

int HvPowerSupply::readVoltage()
{
	int out = hwReadVoltage();
	emit voltageUpdate(out);
	return out;
}

