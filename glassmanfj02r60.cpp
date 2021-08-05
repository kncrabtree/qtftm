#include "glassmanfj02r60.h"

GlassmanFJ02R60::GlassmanFJ02R60() : d_fullScaleVoltage(2000), d_fullScaleCurrent(60), d_currentVoltage(0)
{
    d_subKey = QString("glassmanfj02r60");
    d_prettyName = QString("Glassman FJ02R60 HV Power Supply");
    d_commType = CommunicationProtocol::Rs232;

    d_isCritical = false;


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

    QByteArray resp = sendQueryCmd();	//PBC modified to be the new simple query command
    if(resp.isEmpty())
    {
        emit connected(false,QString("Did not respond to ID Query"));
        return false;
    }

    if(!resp.startsWith('R') || resp.length() != 16)
    {
        emit connected(false,QString("ID Response invalid. Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())));
        return false;
    }

    emit logMessage(QString("ID response: %1").arg(QString(resp)));

    readVoltage();

    emit connected();
    return true;


}

void GlassmanFJ02R60::initialize()
{
    p_comm->setReadOptions(1000,true,QByteArray("\r"));
    p_comm->initialize();
    testConnection();
    //disableTimeoutCmd();    //See warning about the safety of this command in its description
}

bool GlassmanFJ02R60::hwSetVoltage(int v)
{
    int scaledV = v*0xfff;
    scaledV /= d_fullScaleVoltage;

    int tries = 0, maxTries = 3;
    int res = -1;
    while(tries < maxTries)
    {
        if(!checkFault())
            break;

        res = sendSetCommand(QString("S%1FFF0000002").arg(scaledV,3,16,QChar('0')));
        if(res == 5 || res == 6)
        {
            tries++;
            emit logMessage(QString("Trying again to set voltage (%1/%2)").arg(tries+1).arg(maxTries));
            res = sendSetCommand(QString("S%1FFF0000002").arg(scaledV,3,16,QChar('0')));
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
    {
        bool done = false;
        int max_tries = 10;
        int i = 0;
        while(!done && i < max_tries)
        {
            i++;
            p_comm->device()->waitForReadyRead(250);
            int vr = hwReadVoltage();
            if(v < 0)
                return false;
            if(qAbs(v-vr) < 30)
                done = true;
        }

        if(!done)
            emit logMessage(QString("Voltage has not settled after %1 tries.").arg(max_tries),QtFTM::LogWarning);

        return true;
    }
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
    if(!ok)
    {
        emit logMessage(QString("Could not parse voltage. Response: %1 (Hex: %2)").arg(QString(resp).arg(QString(resp.toHex()))),QtFTM::LogError);
        return -1;
    }
    d_currentVoltage = v;
    return v;

}

QByteArray GlassmanFJ02R60::calculateChecksum(QByteArray cmd)
{
    quint8 checksum = 0;
    for(int i=0; i<cmd.length(); i++)
        checksum += cmd.at(i);

    return QString("%1").arg(checksum,2,16,QChar('0')).toLatin1().toUpper();
}

QString GlassmanFJ02R60::formatMessage(QString cmd)
{
    QByteArray dat = cmd.toLatin1().toUpper();
    dat.append(calculateChecksum(dat));
    dat.prepend(1);
    dat.append("\r");
    return QString(dat.toUpper());
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
	QString Test = QString("Q51");
	QByteArray dat = Test.toLatin1().toUpper();
	dat.prepend(1);
    dat.append("\r");
    QByteArray QueryCmd; //PBC Hard coding the query command, theres only one you can send
    QueryCmd.resize(5);
    QueryCmd[0] = 0x01;
    QueryCmd[1] = 0x51;
    QueryCmd[2] = 0x35;
    QueryCmd[3] = 0x31;
    QueryCmd[4] = 0x0d;
    QByteArray Test2 = QByteArrayLiteral("\x01\x51\x35\x31\x0d");	//Testing a cleaner version
    //for (int i; i<5; i++) printf ("%d\n", (int) Test[i]);
    return p_comm->queryCmd(QString(dat.toUpper()));
}

bool GlassmanFJ02R60::disableTimeoutCmd()
{
    /*
     * Note: the timeout is a safety feature from the manufacturer to prevent unsupervised operation in remote mode
     * It is against the manufacturer's recommendation to disable it, do so at your own risk, this is afterall a HV power supply
    */
    QString Test = QString("C174");
	QByteArray dat = Test.toLatin1().toUpper();
	dat.prepend(1);
    dat.append("\r");
    QByteArray Resp = p_comm->queryCmd(QString(dat.toUpper()));
    if (Resp.startsWith('A')) return true;
	else return false;
}

bool GlassmanFJ02R60::enableTimeoutCmd()
{
    /*
     * Note: When the comm timeout is enabled it automatically drops the voltate to 0 if the device is in remote mode and not polled approximately every second
     * The code should manage this, but disabling it can produce more reliable control if it isnt polled often enough for some reason.
     * It will be enabled by default
    */
    QString Test = QString("C073");
	QByteArray dat = Test.toLatin1().toUpper();
	dat.prepend(1);
    dat.append("\r");
    QByteArray Resp = p_comm->queryCmd(QString(dat.toUpper()));
    if (Resp.startsWith('A')) return true;
	else return false;
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
        int res = sendSetCommand(QString("S0000000000004"));
        if(res != 0)
        {
            sendErrorMessage(res);
            emit hardwareFailure();
            return false;
        }

    }
    return true;
}

