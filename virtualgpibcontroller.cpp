#include "virtualgpibcontroller.h"

VirtualGpibController::VirtualGpibController(QObject *parent) :
	GpibController(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual GPIB Controller");
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
