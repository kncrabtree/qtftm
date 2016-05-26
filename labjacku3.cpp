#include "labjacku3.h"

LabjackU3::LabjackU3(QObject *parent) :
    IOBoard(parent), d_handle(nullptr), d_serialNo(3), d_cwLine(16), d_highBandLine(17),
    d_magnetLine(18), d_counterPinOffset(4), d_blockTriggering(false), d_counterCount(0)
{
    d_subKey = QString("labjacku3");
    d_prettyName = QString("LabJack U3 IO Board");
    d_commType = CommunicationProtocol::Custom;
}



bool LabjackU3::testConnection()
{
    if(p_readTimer->isActive())
        p_readTimer->stop();

    if(d_handle != nullptr)
        closeConnection();

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    d_serialNo = s.value(QString("serialNo"),3).toInt();
    s.endGroup();
    s.endGroup();

    //open labjack device with indicated serial number. If serial number is 3, this will open the first LabJack U3 device found.
    d_handle = openUSBConnection(d_serialNo);
    if(d_handle == nullptr)
    {
        emit connected(false,QString("Could not open USB connection."));
        return false;
    }

    //get configuration info
    if(getCalibrationInfo(d_handle,&d_calInfo)< 0)
    {
        closeConnection();
        emit connected(false,QString("Could not retrieve calibration info."));
        return false;
    }

    emit connected();
    emit logMessage(QString("ID response: %1").arg(d_calInfo.prodID));

    configure();

    p_readTimer->start(5);
    return true;
}

void LabjackU3::initialize()
{
    testConnection();
}

long LabjackU3::setCwMode(bool cw)
{
    if(d_handle == nullptr)
    {
        emit logMessage(QString("Not connected, so cw mode cannot be set!"));
        emit hardwareFailure();
        return -1;
    }
    //cw line is CIO0 (16) by default. In cw mode, this output is low.
    long error = 0;
    if(cw)
        error = eDO(d_handle,0,d_cwLine,0);
    else
        error = eDO(d_handle,0,d_cwLine,1);

    if(error)
    {
        emit hardwareFailure();
        emit logMessage(QString("Error while toggling CW mode. Error code: %1").arg(error));
        return -1;
    }

    if(cw)
    {
        d_blockTriggering = true;
        return 1;
    }
    else
    {
        d_blockTriggering = false;
        return 0;
    }
}

void LabjackU3::ftmSynthBand(int band)
{
    if(d_handle == nullptr)
    {
        emit logMessage(QString("Not connected, so low/high band cannot be set!"));
        emit hardwareFailure();
        return;
    }

    if(band < 0 || band > 1)
    {
        emit logMessage(QString("Invalid band (%2)!").arg(band));
        emit hardwareFailure();
        return;
    }


    //for reasons that boggle the mind, high band is 0, and low band is 1. Blame Paul. It's his fault.
    long error = 0;
    if(band == 1)
        error = eDO(d_handle,0,d_highBandLine,0);
    else
        error = eDO(d_handle,0,d_highBandLine,1);

    if(error)
    {
        emit hardwareFailure();
        emit logMessage(QString("Error while toggling low/high band. Error code: %1").arg(error));
        return;
    }

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.setValue(QString("%1/%2/ftmSynthBand").arg(d_key).arg(d_subKey),band);

}

long LabjackU3::setMagnet(bool mag)
{
    if(d_handle == nullptr)
    {
        emit logMessage(QString("Not connected, so magnet cannot be set!"));
        emit hardwareFailure();
        return -1;
    }
    //magnet line is CIO2 (18) by default. If magnet is on, this channel goes low (cuts power to vertical Helmholtz coil)
    long error = 0;
    if(mag)
        error = eDO(d_handle,0,d_magnetLine,0);
    else
        error = eDO(d_handle,0,d_magnetLine,1);

    if(error)
    {
        emit hardwareFailure();
        emit logMessage(QString("Error while toggling magnet. Error code: %1").arg(error));
        return -1;
    }

    emit magnetUpdate(mag);

    if(mag)
        return 1;

    return 0;
}

void LabjackU3::checkForTrigger()
{
    if(d_handle == nullptr)
        return;

    quint32 count = readCounter();
    if(count != d_counterCount)
    {
        d_counterCount = count;
        if(!d_blockTriggering)
            emit triggered();
    }
}

void LabjackU3::configure()
{
    if(d_handle == nullptr)
        return;

    if(p_readTimer->isActive())
        p_readTimer->stop();

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    d_cwLine = s.value(QString("cwLine"),16).toInt();
    d_highBandLine = s.value(QString("highBandLine"),17).toInt();
    d_magnetLine = s.value(QString("magnetLine"),18).toInt();
    d_counterPinOffset = s.value(QString("counterOffset"),4).toInt();


    //enable counter on indicated line
    long enableTimers[2] = {0,0}, enableCounters[2] = {1,0}, timerModes[2] = {0,0};
    double timerValues[2] = {0.0,0.0};
    eTCConfig(d_handle,enableTimers,enableCounters,d_counterPinOffset,LJ_tc48MHZ,0,timerModes,timerValues,0,0);

    //reset counter
    resetCounter();

    int band = s.value(QString("ftmSynthBand"),0).toInt();
    s.endGroup();
    s.endGroup();
    setCwMode(false);
    ftmSynthBand(band);
    setMagnet(false);

    p_readTimer->start(5);
}

void LabjackU3::resetCounter()
{
    long readTimers[2] = {0,0}, resetTimers[2] = {0,0}, readCounters[2] = {0,0}, resetCounters[2] = {1,0};
    double timerValues[2] = {0.0,0.0}, counterValues[2] = {0,0};
    long error = eTCValues(d_handle,readTimers,resetTimers,readCounters,resetCounters,timerValues,counterValues,0,0);
    if(error)
    {
        p_readTimer->stop();
        emit hardwareFailure();
        emit logMessage(QString("Error resetting %1 counter. Error code %2").arg(d_prettyName).arg(error),QtFTM::LogError);
    }
    d_counterCount = 0;
}

quint32 LabjackU3::readCounter()
{
    long readTimers[2] = {0,0}, resetTimers[2] = {0,0}, readCounters[2] = {1,0}, resetCounters[2] = {0,0};
    double timerValues[2] = {0.0,0.0}, counterValues[2] = {0,0};
    long error = eTCValues(d_handle,readTimers,resetTimers,readCounters,resetCounters,timerValues,counterValues,0,0);
    if(error)
    {
        p_readTimer->stop();
        emit hardwareFailure();
        emit logMessage(QString("Error reading %1 counter. Error code %2").arg(d_prettyName).arg(error),QtFTM::LogError);
    }
    return static_cast<quint32>(counterValues[0]);
}

void LabjackU3::closeConnection()
{
    closeUSBConnection(d_handle);
    d_handle = nullptr;
}
