#include "hardwareobject.h"

HardwareObject::HardwareObject(QObject *parent) :
    QObject(parent), d_isCritical(true), d_isConnected(false)
{
}


void HardwareObject::sleep(bool b)
{
	if(b)
		emit logMessage(name().append(QString(" is asleep.")));
	else
		emit logMessage(name().append(QString(" is awake.")));
}
