#include "aeroflexattn.h"

#include "tcpinstrument.h"

AeroflexAttn::AeroflexAttn(QObject *parent) :
    Attenuator(parent)
{
    d_subKey = QString("aeroflex");
    d_prettyName = QString("Aeroflex Weinschel Attenuator");
    d_commType = CommunicationProtocol::Tcp;
    d_threaded = false;


    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    d_min = s.value(QString("%1/%2/min").arg(d_key).arg(d_subKey),0).toInt();
    d_max = s.value(QString("%1/%2/max").arg(d_key).arg(d_subKey),100).toInt();
    //rewrite these values (so that they always appear in file)
    s.setValue(QString("%1/%2/min").arg(d_key).arg(d_subKey),d_min);
    s.setValue(QString("%1/%2/max").arg(d_key).arg(d_subKey),d_max);

    //the values below are used by the program to set limits, and will be overwritten by other implementations
    //at runtime if they are selected at compile time
    s.setValue(QString(d_key).append("/min"),d_min);
    s.setValue(QString(d_key).append("/max"),d_max);
    s.sync();
}


bool AeroflexAttn::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    QByteArray resp = p_comm->queryCmd(QString("*IDN?\n"));
    if(resp.isEmpty())
    {
        emit connected(false,QString("%Null response to ID query"));
        return false;
    }
    if(!resp.startsWith("Aeroflex Weinschel"))
    {
        emit connected(false,QString("ID response invalid. Received: %1").arg(QString(resp)));
        return false;
    }

    emit logMessage(QString("ID response: %2").arg(QString(resp)));


    if(readAttn() < 0)
    {
        emit connected(false);
        return false;
    }

    emit connected();
    return true;
}

void AeroflexAttn::initialize()
{
    Attenuator::initialize();

    p_comm->setReadOptions(1000,true,QByteArray("\r"));
    p_comm->initialize();
    testConnection();
}

int AeroflexAttn::setAttn(int a)
{
    //write attenuation command
    //changing attenuation takes some time, so wait for *OPC? to return before reading
    if(a < d_min || a > d_max)
    {
	    emit logMessage(QString("Requested attenuation (%1) is outside valid range (%2 - %3).").arg(a).arg(d_min).arg(d_max));
	    return -1;
    }

    p_comm->queryCmd(QString("ATTN 1 %1;*OPC?\n").arg(a)).trimmed();
    return readAttn();
}

int AeroflexAttn::readAttn()
{
    //write query
    QByteArray resp = p_comm->queryCmd(QString("ATTN? 1\n")).trimmed();
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("Did not respond to attenuation query."),QtFTM::LogError);
        return -1;
    }
    bool ok = false;
    int a = resp.toInt(&ok);
    if(!ok || a < 0)
    {
        emit hardwareFailure();
        emit logMessage(QString("Invalid response to attenuation query (response: %1)").arg(QString(resp)),QtFTM::LogError);
        return -1;
    }

    emit attnUpdate(a);
    return a;
}
