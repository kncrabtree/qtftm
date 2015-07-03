#include "virtualinstrument.h"

VirtualInstrument::VirtualInstrument(QString key, QString subKey, QObject *parent) :
	CommunicationProtocol(CommunicationProtocol::Virtual,key,subKey,parent)
{

}

VirtualInstrument::~VirtualInstrument()
{

}



bool VirtualInstrument::writeCmd(QString cmd)
{
	Q_UNUSED(cmd)
	return true;
}

bool VirtualInstrument::writeBinary(QByteArray dat)
{
	Q_UNUSED(dat)
	return true;
}

QByteArray VirtualInstrument::queryCmd(QString cmd)
{
	Q_UNUSED(cmd)
	return QByteArray();
}

QIODevice *VirtualInstrument::device()
{
	return nullptr;
}

void VirtualInstrument::initialize()
{
}

bool VirtualInstrument::testConnection()
{
	return true;
}
