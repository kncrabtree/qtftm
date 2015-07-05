#include "virtualmotordriver.h"

#include "virtualinstrument.h"

VirtualMotorDriver::VirtualMotorDriver(QObject *parent) :
	MotorDriver(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual Motor Driver");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);
}

VirtualMotorDriver::~VirtualMotorDriver()
{

}

bool VirtualMotorDriver::testConnection()
{
	emit connected();
	return true;
}

void VirtualMotorDriver::initialize()
{
	MotorDriver::initialize();
	testConnection();
}

bool VirtualMotorDriver::moveToPosition(int pos)
{
	Q_UNUSED(pos)
	return true;
}

bool VirtualMotorDriver::stepMotor(int motorSteps)
{
	Q_UNUSED(motorSteps)
	return true;
}

int VirtualMotorDriver::readAnalog()
{
	return 900 + (qrand()%200);
}

int VirtualMotorDriver::readPos()
{
	return 0;
}


void VirtualMotorDriver::tune(double freq, int currentAttn, int mode)
{
	d_lastTuneFreq = freq;
	d_lastTuneAtten = currentAttn;
	Q_UNUSED(mode)

	d_lastTuneVoltage = readAnalog();
	emit voltageChanged(d_lastTuneVoltage);

	emit tuningComplete(true);
}

void VirtualMotorDriver::calibrate()
{
	d_lastCalVoltage = readAnalog();
	emit voltageChanged(d_lastCalVoltage);

	emit tuningComplete(true);
}
