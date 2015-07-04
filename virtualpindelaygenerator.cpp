#include "virtualpindelaygenerator.h"

#include "virtualinstrument.h"

VirtualPinDelayGenerator::VirtualPinDelayGenerator(QObject *parent) :
	PinSwitchDriveDelayGenerator(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual Pin Switch Delay Generator");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(d_key);
	s.beginGroup(d_subKey);
	int minProt = s.value(QString("minProt"),1).toInt();
	int maxProt = s.value(QString("maxProt"),100).toInt();
	int minScope = s.value(QString("minScope"),0).toInt();
	int maxScope = s.value(QString("maxScope"),100).toInt();
	s.setValue(QString("minProt"),minProt);
	s.setValue(QString("maxProt"),maxProt);
	s.setValue(QString("minScope"),minScope);
	s.setValue(QString("maxScope"),maxScope);
	s.endGroup();
	s.endGroup();
	s.sync();

	d_currentProtectionDelay = 1;
	d_currentScopeDelay = 0;
}

VirtualPinDelayGenerator::~VirtualPinDelayGenerator()
{

}



bool VirtualPinDelayGenerator::testConnection()
{
	readAll();
	emit connected(true);
	return true;
}

void VirtualPinDelayGenerator::initialize()
{
	testConnection();
}

int VirtualPinDelayGenerator::setProtectionDelay(int delayInMicroseconds)
{
	d_currentProtectionDelay = delayInMicroseconds;
	readAll();
	return delayInMicroseconds;
}

int VirtualPinDelayGenerator::setScopeDelay(int delayInMicroscconds)
{
	d_currentScopeDelay = delayInMicroscconds;
	readAll();
	return delayInMicroscconds;
}

void VirtualPinDelayGenerator::readAll()
{
	emit protectionDelayUpdate(d_currentProtectionDelay);
	emit scopeTriggerDelayUpdate(d_currentScopeDelay);
}
