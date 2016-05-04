#include "glassmanfj02r60.h"

#include "tcpinstrument.h"

GlassmanFJ02R60::GlassmanFJ02R60() : d_fullScaleVoltage(2000), d_fullScaleCurrent(60), d_currentVoltage(0)
{
    d_subKey = QString("glassmanfj02r60");
    d_prettyName = QString("Glassman FJ02R60 HV Power Supply");

    d_isCritical = false;

    p_comm = new TcpInstrument(d_key,d_subKey,this);
    connect(p_comm,&CommunicationProtocol::logMessage,this,&GlassmanFJ02R60::logMessage);
    connect(p_comm,&CommunicationProtocol::hardwareFailure,this,&GlassmanFJ02R60::hardwareFailure);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    d_hardwareMin = s.value(QString("%1/%2/min").arg(d_key).arg(d_subKey),0).toInt();
    d_hardwareMax = s.value(QString("%1/%2/max").arg(d_key).arg(d_subKey),d_fullScaleVoltage).toInt();
    //rewrite these values (so that they always appear in file)
    s.setValue(QString("%1/%2/min").arg(d_key).arg(d_subKey),d_hardwareMin);
    s.setValue(QString("%1/%2/max").arg(d_key).arg(d_subKey),d_hardwareMax);
}



bool GlassmanFJ02R60::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    QByteArray telnetOptions;
    telnetOptions.append(255);
    telnetOptions.append(253);
    telnetOptions.append(3);
    telnetOptions.append(255);
    telnetOptions.append(252);
    telnetOptions.append(1);

    p_comm->queryCmd(QString(telnetOptions));

    QByteArray resp = p_comm->queryCmd(formatMessage(QString("V")));

    if(resp.isEmpty())
    {
        emit connected(false,QString("Did not respond to ID Query"));
        return false;
    }

    if(!resp.startsWith('B') || resp.length() != 6)
    {
        emit connected(false,QString("ID Response invalid. Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())));
        return false;
    }

    emit logMessage(QString("ID response: %1").arg(QString(resp)));

    if(!sendSetCommand(QString("C1")))
    {
        emit connected(false,QString("Could not disable timeout feature."));
        return false;
    }

    emit connected();
    return true;


}

void GlassmanFJ02R60::initialize()
{
    p_comm->setReadOptions(1000,true,QByteArray("\r"));
    p_comm->initialize();
    testConnection();
}

bool GlassmanFJ02R60::hwSetVoltage(int v)
{
    int scaledV = v*0xfff;
    scaledV /= d_fullScaleVoltage;
    QString vString = QString::number(scaledV,16).toUpper();

    int tries = 0, maxTries = 3;
    int res = -1;
    while(tries < maxTries)
    {
        if(!checkFault())
            break;

        res = sendSetCommand(QString("S%1FFF000002").arg(vString));
        if(res == 5 || res == 6)
        {
            tries++;
            emit logMessage(QString("Trying again to set voltage (%1/%2)").arg(tries+1).arg(maxTries));
            res = sendSetCommand(QString("S%1FFF000002").arg(vString));
        }
        else
            break;
    }

    if(res != 0)
    {
        sendErrorMessage(res);
        emit hardwareFailure();
        return false;
    }
    else
        return true;
}

int GlassmanFJ02R60::hwReadVoltage()
{
    QByteArray resp = sendQueryCmd();
    if(!resp.startsWith("R") || resp.length() < 16)
    {
        emit logMessage(QString("Could not query device. Response: %1 (hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        emit hardwareFailure();
        return false;
    }

    bool ok = false;
    int v = resp.mid(1,3).toInt(&ok,16)*d_fullScaleVoltage;
    v /= 0x3ff;

    d_currentVoltage = v;
    return v;

}

QByteArray GlassmanFJ02R60::calculateChecksum(QByteArray cmd)
{
    quint8 checksum = 0;
    for(int i=0; i<cmd.length(); i++)
        checksum += cmd.at(i);

    return QString("%1").arg(checksum,2,16,QChar('0')).toLatin1();
}

QString GlassmanFJ02R60::formatMessage(QString cmd)
{
    QByteArray dat;
    dat.append(1);
    dat.append(cmd.toLatin1());
    dat.append(calculateChecksum(cmd.toLatin1()));
    dat.append("\r");
    return dat;
}

int GlassmanFJ02R60::sendSetCommand(QString cmd)
{
    QByteArray resp = p_comm->queryCmd(formatMessage(cmd));
    if(resp.startsWith('A'))
        return 0;

    if(resp.startsWith('E'))
    {
        emit logMessage(QString("Received error code when sending command \"%1\" (hex = %2)").arg(cmd).arg(QString(cmd.toLatin1().toHex())),QtFTM::LogWarning);
        if(resp.length() >= 2)
        {
            bool ok = false;
            int errCode = resp.mid(1,1).toInt(&ok);
            if(ok)
                return errCode;
            else
                return -1;
        }
    }

    return -1;
}

QByteArray GlassmanFJ02R60::sendQueryCmd()
{
    return p_comm->queryCmd(formatMessage("Q"));
}

void GlassmanFJ02R60::sendErrorMessage(int errCode)
{
    switch(errCode)
    {
    case 1:
        emit logMessage(QString("Error code 1: Undefined command"),QtFTM::LogError);
        break;
    case 2:
        emit logMessage(QString("Error code 2: Checksum error."),QtFTM::LogError);
        break;
    case 3:
        emit logMessage(QString("Error code 3: Extra bytes error"),QtFTM::LogError);
        break;
    case 4:
        emit logMessage(QString("Error code 4: Illegal digital control byte"),QtFTM::LogError);
        break;
    case 5:
        emit logMessage(QString("Error code 5: Fault active."),QtFTM::LogError);
        break;
    case 6:
        emit logMessage(QString("Error code 6: Processing error."),QtFTM::LogError);
        break;
    default:
        emit logMessage(QString("Cound not determine reason for error."));
        break;
    }
}

bool GlassmanFJ02R60::checkFault()
{
    QByteArray resp = sendQueryCmd();
    if(!resp.startsWith("R") || resp.length() < 16)
    {
        emit logMessage(QString("Could not query device. Response: %1 (hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        emit hardwareFailure();
        return false;
    }

    if(resp.at(10) == 2) //fault active; clear and reset
    {
        int res = sendSetCommand(QString("S000000000004"));
        if(res != 0)
        {
            sendErrorMessage(res);
            emit hardwareFailure();
            return false;
        }

    }

    return true;
}

