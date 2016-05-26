#include "virtualhvps.h"

VirtualHVPS::VirtualHVPS(QObject *parent) :
	HvPowerSupply(parent), d_currentVoltage(0)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual HV Power Supply");

	//this code should be copied into all implementations; replace limits appropriately
	QSettings s(QSettings::SystemScope, QApplication::organizationName(), QApplication::applicationName());
	s.beginGroup(d_key);
	s.beginGroup(d_subKey);
	d_hardwareMin = s.value(QString("min"),0).toInt();
	d_hardwareMax = s.value(QString("max"),2000).toInt();
	s.setValue(QString("min"),d_hardwareMin);
	s.setValue(QString("max"),d_hardwareMax);
	s.endGroup();
	s.endGroup();
}

VirtualHVPS::~VirtualHVPS()
{

}

bool VirtualHVPS::testConnection()
{
	readVoltage();
	emit connected(true);
	return true;
}

void VirtualHVPS::initialize()
{
	testConnection();
}

bool VirtualHVPS::hwSetVoltage(int v)
{
	d_currentVoltage = v;
    return true;
}

int VirtualHVPS::hwReadVoltage()
{
	return d_currentVoltage;
}
