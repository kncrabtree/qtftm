#include "virtualattenuator.h"

#include "virtualinstrument.h"

VirtualAttenuator::VirtualAttenuator(QObject *parent) :
	Attenuator(parent), d_currentAttn(0)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual Attenuator");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);
}

VirtualAttenuator::~VirtualAttenuator()
{

}

void VirtualAttenuator::initialize()
{
	Attenuator::initialize();
	testConnection();
}

bool VirtualAttenuator::testConnection()
{
	readAttn();
	emit connected();
	return true;
}

int VirtualAttenuator::setAttn(int a)
{
	d_currentAttn = a;
	return readAttn();
}

int VirtualAttenuator::readAttn()
{
	emit attnUpdate(d_currentAttn);
	return d_currentAttn;
}
