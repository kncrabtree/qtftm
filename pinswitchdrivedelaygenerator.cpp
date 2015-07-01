#include "pinswitchdrivedelaygenerator.h"

#include <QStringList>

PinSwitchDriveDelayGenerator::PinSwitchDriveDelayGenerator (QObject *parent) :
    Rs232Instrument(QString("pdg"),QString("Pin Switch Delay Generator"),parent), d_currentScopeDelay(5), d_currentProtectionDelay(5), d_quiet(false)
{
}


void PinSwitchDriveDelayGenerator::initialize()
{
    setReadOptions(100,true,QByteArray("\n\r"));
    Rs232Instrument::initialize();
    testConnection();
    //emit canTuneDown(false); model of how to do this
    //emit canTuneUp(false);
}

bool PinSwitchDriveDelayGenerator::testConnection()
{
    if(!Rs232Instrument::testConnection())
    {
        emit connected(false,QString("RS232 error. Verify that the device mapping has not changed (see rs232instrument.h) and that port settings are correct."));
        return false;
    }

    if(d_sp->bytesAvailable())
        d_sp->readAll();

    QByteArray resp = queryCmd(QString("I"));
    if(!resp.startsWith("I:PIN"))
    {
        emit connected(false,QString("Error communicating with %1. Verify that the device mapping has not changed (see rs232instrument.h) and that port settings are correct.").arg(d_prettyName));
        return false;
    }
    // readAll will find out the current settings and set them up in the user interface.
    readAll();
    emit connected();
    return true;
}



int PinSwitchDriveDelayGenerator::currentScopeDelay() const
{
    return d_currentScopeDelay;
}

int PinSwitchDriveDelayGenerator::currentProtectionDelay() const
{
    return d_currentProtectionDelay;
}




void PinSwitchDriveDelayGenerator::readAll()

{
    //QPair<int,int> out(-1,-1);
    QByteArray resp = queryCmd(QString("S")).trimmed();// 'S' = get status, returns S:Delay A =%d, Delay B =%d\n\r
    if(resp.isEmpty() || !resp.startsWith("S:"))
    {
        emit hardwareFailure();
        emit logMessage(QString("Communication error with %1 while reading delay generator state").arg(d_prettyName),LogHandler::Error);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),LogHandler::Error);
        return;
    }

    bool ok = false;
    QStringList l = QString(resp.trimmed()).split(",");// modify this for two variables, 1st = protection delay, 2nd = scope delay

    int monitorDelay = l.at(0).split("=").at(1).trimmed().toInt(&ok);
    int scopeDelay = -1;
    if(ok)
        scopeDelay = l.at(1).split("=").at(1).trimmed().toInt(&ok);

    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse response from %1.").arg(d_prettyName),LogHandler::Error);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),LogHandler::Error);
        return;
    }

    emit protectionDelayUpdate(monitorDelay);
    emit scopeTriggerDelayUpdate(scopeDelay);

    d_currentProtectionDelay = monitorDelay;
    d_currentScopeDelay = scopeDelay;

    return;
}

int PinSwitchDriveDelayGenerator::setProtectionDelay(int delayInMicroseconds)
{
    if(!sendDelay(Protection, delayInMicroseconds))
        return -1;

    readAll();
    return(d_currentProtectionDelay);
}

int PinSwitchDriveDelayGenerator::setScopeDelay(int delayInMicroseconds)
{
    if(!sendDelay(Scope, delayInMicroseconds))
        return -1;

    readAll();
    return(d_currentScopeDelay);
}

bool PinSwitchDriveDelayGenerator::sendDelay(Channel ch, int delayInMicroseconds)
{

    QByteArray resp;
    if(ch == Protection)
        resp = queryCmd(QString("A%1\n").arg(delayInMicroseconds));
    else
        resp = queryCmd(QString("B%1\n").arg(delayInMicroseconds));



    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("Communication error with %1 while setting delay.").arg(d_prettyName),LogHandler::Error);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),LogHandler::Error);
        return false;
    }
    return true;
}





