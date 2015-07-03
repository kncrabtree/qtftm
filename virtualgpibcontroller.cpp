#include "virtualgpibcontroller.h"

#include "virtualinstrument.h"

VirtualGpibController::VirtualGpibController(QObject *parent) :
	GpibController(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual GPIB Controller");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);
}

VirtualGpibController::~VirtualGpibController()
{

}



bool VirtualGpibController::testConnection()
{
	emit connected();
	return true;
}

void VirtualGpibController::initialize()
{
	testConnection();
}

bool VirtualGpibController::readAddress()
{
	return d_currentAddress;
}

bool VirtualGpibController::setAddress(int a)
{
	d_currentAddress = a;
	return true;
}
