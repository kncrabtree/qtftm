#include "virtualioboard.h"

#include "virtualinstrument.h"

VirtualIOBoard::VirtualIOBoard(QObject *parent) :
	IOBoard(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual IOBoard");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);
}

VirtualIOBoard::~VirtualIOBoard()
{

}



bool VirtualIOBoard::testConnection()
{
	if(!p_readTimer->isActive())
		p_readTimer->start();

	emit connected();
	return true;
}

void VirtualIOBoard::initialize()
{
	p_readTimer->setInterval(167);
	testConnection();
}

long VirtualIOBoard::setCwMode(bool cw)
{
	if(cw)
		return 1;
	else
		return 0;
}

void VirtualIOBoard::ftmSynthBand(int band)
{
	Q_UNUSED(band)
}

long VirtualIOBoard::setMagnet(bool mag)
{
	emit magnetUpdate(mag);
	if(mag)
		return 1;
	else
		return 0;
}

void VirtualIOBoard::checkForTrigger()
{
	emit triggered();
}
