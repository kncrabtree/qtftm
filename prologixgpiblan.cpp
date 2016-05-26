#include "prologixgpiblan.h"

PrologixGpibLan::PrologixGpibLan(QObject *parent) :
    GpibController(parent)
{
    d_subKey = QString("prologixGpibLan");
    d_prettyName = QString("Prologix GPIB-LAN Controller");
    d_commType = CommunicationProtocol::Tcp;
}



bool PrologixGpibLan::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    QByteArray resp = p_comm->queryCmd(QString("++ver\n"));
    if(resp.isEmpty())
    {
        emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
        return false;
    }
    if(!resp.startsWith("Prologix GPIB-ETHERNET Controller"))
    {
        emit connected(false,QString("%1 response invalid. Received: %2").arg(d_prettyName).arg(QString(resp)));
        return false;
    }

    emit logMessage(QString("%1 ID response: %2").arg(d_prettyName).arg(QString(resp)));

    p_comm->writeCmd(QString("++auto 0\n"));
    p_comm->writeCmd(QString("++savecfg 0\n"));
    p_comm->writeCmd(QString("++read_tmo_ms 50\n"));

    readAddress();

    emit connected();
    return true;
}

void PrologixGpibLan::initialize()
{
    p_comm->setReadOptions(1000,true,QByteArray("\n"));
    p_comm->initialize();
    testConnection();
}

bool PrologixGpibLan::readAddress()
{
    bool success = false;
    d_currentAddress = p_comm->queryCmd(QString("++addr\n")).trimmed().toInt(&success);
    return success;
}

bool PrologixGpibLan::setAddress(int a)
{
    if(!p_comm->writeCmd(QString("++addr %1\n").arg(a)))
        return false;

    if(!readAddress())
        return false;

    if(d_currentAddress != a)
    {
        emit logMessage(QString("Address was not set to %1. Current address: %2").arg(a).arg(d_currentAddress),QtFTM::LogError);
        return false;
    }

    return true;
}

QString PrologixGpibLan::queryTerminator() const
{
    return QString("++read eoi\n");
}
