#include "mks946.h"

Mks946::Mks946(QObject *parent) : FlowController(parent), d_nextRead(0)
{
    d_subKey = QString("mks946");
    d_prettyName = QString("MKS 946 Flow Controller");
    d_threaded = false;
    d_commType = CommunicationProtocol::Rs232;
    d_isCritical = false;

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    d_numChannels = s.value(QString("numChannels"),4).toInt();
    d_address = s.value(QString("address"),253).toInt();
    d_pressureChannel = s.value(QString("pressureChannel"),5).toInt();
    d_channelOffset = s.value(QString("channelOffset"),1).toInt();
    s.setValue(QString("numChannels"),d_numChannels);
    s.setValue(QString("address"),d_address);
    s.setValue(QString("pressureChannel"),d_pressureChannel);
    s.setValue(QString("channelOffset"),d_channelOffset);
    s.endGroup();
    s.endGroup();
    s.sync();
}

bool Mks946::testConnection()
{
    //PBC
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }


    QByteArray resp = mksQuery(QString("MD?"));
    if(!resp.contains(QByteArray("946")))
    {
        d_errorString = QString("Received invalid response to model query. Response: %1").arg(QString(resp));
        return false;
    }

    emit logMessage(QString("Response: %1").arg(QString(resp)));

    readAll();
    p_readTimer->start();
    emit connected();
    return true;
}

double Mks946::setFlowSetpoint(const int ch, const double val)
{
    if(!isConnected())
    {
        emit logMessage(QString("Cannot set flow setpoint due to a previous communication failure. Reconnect and try again."),QtFTM::LogError);
        return -1.0;	//PBC returning -1 to mimic 647
    }

    bool pidActive = false;

    //first ensure recipe 1 is active
    if(!mksWrite(QString("RCP!1")))
    {
        if(d_errorString.contains(QString("166")))
        {
            pidActive = true;
            if(!mksWrite(QString("PID!OFF")))
            {
                emit logMessage(QString("Could not disable PID mode to update channel %1 setpoint. Error: %2").arg(ch+1).arg(d_errorString),QtFTM::LogError);
                emit hardwareFailure();
                return -1.0;	//PBC
            }
        }
        else
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return -1.0;	//PBC
        }
    }

    if(!pidActive)
    {
        //make sure ratio recipe 1 is active
        if(!mksWrite(QString("RRCP!1")))
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return -1.0; //PBC
        }
    }

    double val2 = val/2.0;
    if(!mksWrite(QString("RRQ%1!%2").arg(ch+d_channelOffset).arg(val2,0,'E',2,QLatin1Char(' '))))
    {
        emit logMessage(d_errorString,QtFTM::LogError);
        emit hardwareFailure();
        return -1.0;	//PBC
    }

    if(pidActive)
        setPressureControlMode(true);
	
	return readFlowSetpoint(ch);
}

double Mks946::setPressureSetpoint(const double val)
{
    if(!isConnected())
    {
        emit logMessage(QString("Cannot set pressure setpoint due to a previous communication failure. Reconnect and try again."),QtFTM::LogError);
        return -1.0;	//PBC
    }

    bool pidActive = false;

    //first ensure recipe 1 is active
    if(!mksWrite(QString("RCP!1")))
    {
        if(d_errorString.contains(QString("166")))
        {
            pidActive = true;
            if(!mksWrite(QString("PID!OFF")))
            {
                emit logMessage(QString("Could not disable PID mode to change pressure setpoint. Error: %1").arg(d_errorString));
                emit hardwareFailure();
                return -1.0;	//PBC
            }
        }
        else
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return -1.0; //PBC
        }
    }

    if(!mksWrite(QString("RPSP!%1").arg(val*1000.0,1,'E',2,QLatin1Char(' '))))
    {
        emit logMessage(d_errorString,QtFTM::LogError);
        emit hardwareFailure();
        return -1.0; //PBC
    }

    if(pidActive)
        setPressureControlMode(true);

	return readPressureSetpoint();
}

double Mks946::readFlowSetpoint(const int ch)
{
    if(!isConnected())
        return 0.0;

    QByteArray resp = mksQuery(QString("RRQ%1?").arg(ch+d_channelOffset));
    resp.chop(3);
    bool ok = false;
    double out = resp.mid(2).toDouble(&ok);
    if(!ok)
    {
        emit logMessage(QString("Received invalid response to channel %1 setpoint query. Response: %2").arg(ch+1).arg(QString(resp)),QtFTM::LogError);
        emit hardwareFailure();
        return -1.0;
    }
    d_config.set(ch,QtFTM::FlowSettingSetpoint,out);
    emit flowSetpointUpdate(ch,out);
    return out;
}

double Mks946::readPressureSetpoint()
{
    if(!isConnected())
        return 0.0;

    QByteArray resp = mksQuery(QString("RPSP?"));
    resp.chop(3);
    bool ok = false;
    double out = resp.mid(2).toDouble(&ok);
    if(!ok)
    {
        emit logMessage(QString("Received invalid response to pressure setpoint query. Response: %1").arg(QString(resp)),QtFTM::LogError);
        emit hardwareFailure();
        return -1.0;
    }
    out /= 1000.0;
    d_config.setPressureSetpoint(out);
    emit pressureSetpointUpdate(out);
    return out; // convert to kTorr
}

double Mks946::readFlow(const int ch)
{
    if(!isConnected())
        return 0.0;
    QByteArray resp = mksQuery(QString("FR%1?").arg(ch+d_channelOffset));
    if(resp.contains(QByteArray("MISCONN")))
        return 0.0;

    bool ok = false;
    resp.chop(3);
    if(resp.contains(QByteArray("RRQ"))) resp = resp.mid(5,-1);
    if (resp.contains(QByteArray(">"))) {
        emit logMessage(QString("Received invalid response to flow query for channel %1. Response: %2").arg(ch+1).arg(QString(resp)),QtFTM::LogWarning);
        resp.remove(0,1);   //Remove the > and move on
    }
    double out = resp.toDouble(&ok);
    if(!ok)
    {
        emit logMessage(QString("Received invalid response to flow query for channel %1. Response: %2").arg(ch+1).arg(QString(resp)),QtFTM::LogError);
        emit hardwareFailure();
        return -1.0;
    }
    d_config.set(ch,QtFTM::FlowSettingFlow,out);
    emit flowUpdate(ch,out);
    return out;
}

double Mks946::readPressure()
{
    if(!isConnected())
        return 0.0;

    QByteArray resp = mksQuery(QString("PR%1?").arg(d_pressureChannel));
    if(resp.contains(QByteArray("LO")))
        return 0.0;

    if(resp.contains(QByteArray("MISCONN")))
    {
        emit logMessage(QString("No pressure gauge connected."),QtFTM::LogWarning);
        setPressureControlMode(false);
        emit hardwareFailure();
        return -0.0;
    }

    if(resp.contains(QByteArray("ATM")))
        return 10.0;
    bool ok = false;
    resp.chop(3);
    double out = resp.toDouble(&ok);
    if(ok)
        out /= 1000.0;  //convert to kTorr
        d_config.setPressure(out);
        emit pressureUpdate(d_config.pressure());
        return d_config.pressure();

    emit logMessage(QString("Could not parse reply to pressure query. Response: %1").arg(QString(resp)),QtFTM::LogError);
    emit hardwareFailure();
    return -1.0;
}

void Mks946::setPressureControlMode(bool enabled)
{
    if(!isConnected())
    {
        emit logMessage(QString("Cannot set pressure control mode due to a previous communication failure. Reconnect and try again."),QtFTM::LogError);
        return;
    }

    if(enabled)
    {
        //first ensure recipe 1 is active
        if(!mksWrite(QString("RCP!1")))
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return;
        }

        if(!mksWrite(QString("RRCP!1")))
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return;
        }

        QList<QString> chNames;
        chNames << QString("A1") << QString("A2") << QString("B1") << QString("B2") << QString("C1") << QString("C2");

        //ensure pressure sensor is set to control channel
        if(!mksWrite(QString("RPCH!%1").arg(chNames.at(d_pressureChannel-1))))
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return;
        }

        if(!mksWrite(QString("RDCH!Rat")))
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return;
        }

        if(!mksWrite(QString("PID!ON")))
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return;
        }
    }
    else
    {
        if(!mksWrite(QString("PID!OFF")))
        {
            emit logMessage(d_errorString,QtFTM::LogError);
            emit hardwareFailure();
            return;
        }
    }
    readPressureControlMode();  //PBC
}

bool Mks946::readPressureControlMode()
{
    if(!isConnected())
    {
        return -1;
    }
    QByteArray resp = mksQuery(QString("PID?"));
    if(resp.contains(QByteArray("ON")))
        d_config.setPressureControlMode(true);	//PBC
    else if(resp.contains(QByteArray("OFF")))
		d_config.setPressureControlMode(false);	//PBC
    else
        emit logMessage(QString("Received invalid response to pressure control mode query. Response: %1").arg(QString(resp)),QtFTM::LogError);
	emit pressureControlMode(d_config.pressureControlMode());	//PBC
    return d_config.pressureControlMode();	//PBC

}

void Mks946::poll()
{
    if(d_nextRead < 0 || d_nextRead >= d_numChannels)
    {
        readPressure();
//        readPressureSetpoint();
        d_nextRead = 0;
    }
    else
    {
        readFlow(d_nextRead);
//        readFlowSetpoint(d_nextRead);
        d_nextRead++;
    }
}

void Mks946::initialize()
{
    FlowController::initialize();
    p_comm->setReadOptions(100,true,QByteArray(";FF"));
    p_comm->initialize();   //PBC
    testConnection();       //PBC
}

bool Mks946::mksWrite(QString cmd)
{
    QByteArray resp = p_comm->queryCmd(QString("@%1%2;FF").arg(d_address,3,10,QChar('0')).arg(cmd));
    if(resp.contains(QByteArray("ACK")))
        return true;
    d_errorString = QString("Received invalid response to command %1. Response: %2").arg(cmd).arg(QString(resp));
    return false;
}

QByteArray Mks946::mksQuery(QString cmd)
{
    QByteArray resp = p_comm->queryCmd(QString("@%1%2;FF").arg(d_address,3,10,QChar('0')).arg(cmd));
    if(!resp.startsWith(QString("@%1ACK").arg(d_address,3,10,QChar('0')).toLatin1()))
        return resp;

    //chop off prefix
    return resp.mid(7);
}

void Mks946::readSettings()
{
    //numChannels has to be set on program startup, so don't include it here
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    d_address = s.value(QString("address"),253).toInt();
    d_pressureChannel = s.value(QString("pressureChannel"),5).toInt();
    d_channelOffset = s.value(QString("channelOffset"),1).toInt();
    s.setValue(QString("address"),d_address);
    s.setValue(QString("pressureChannel"),d_pressureChannel);
    s.setValue(QString("channelOffset"),d_channelOffset);
    s.endGroup();
    s.endGroup();
}

void Mks946::sleep(bool b)
{
    if(b)
        setPressureControlMode(false);
   HardwareObject::sleep(b);
}
