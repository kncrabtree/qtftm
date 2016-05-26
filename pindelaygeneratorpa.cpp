#include "pindelaygeneratorpa.h"

PinDelayGeneratorPA::PinDelayGeneratorPA(QObject *parent) :
    PinSwitchDriveDelayGenerator(parent)
{
    d_subKey = QString("antonucci");
    d_prettyName = QString("Antonucci Pin Switch Delay Generator");
    d_commType = CommunicationProtocol::Rs232;

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    int minProt = s.value(QString("minProt"),1).toInt();
    int maxProt = s.value(QString("maxProt"),100).toInt();
    int minScope = s.value(QString("minScope"),0).toInt();
    int maxScope = s.value(QString("maxScope"),100).toInt();
    s.setValue(QString("minProt"),minProt);
    s.setValue(QString("maxProt"),maxProt);
    s.setValue(QString("minScope"),minScope);
    s.setValue(QString("maxScope"),maxScope);
    s.endGroup();
    s.endGroup();
    s.sync();
}



bool PinDelayGeneratorPA::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    QByteArray resp = p_comm->queryCmd(QString("I"));
    if(!resp.startsWith("I:PIN"))
    {
        emit connected(false,QString("Invalid ID response. (Response: %1)").arg(QString(resp)));
        return false;
    }

    // readAll will find out the current settings and set them up in the user interface.
    readAll();
    emit connected();
    return true;
}

void PinDelayGeneratorPA::initialize()
{
    p_comm->setReadOptions(100,true,QByteArray("\n\r"));
    p_comm->initialize();
    testConnection();
}

int PinDelayGeneratorPA::setProtectionDelay(int delayus)
{
    if(!sendDelay(Protection, delayus))
        return -1;

    readAll();
    return(d_currentProtectionDelay);
}

int PinDelayGeneratorPA::setScopeDelay(int delayus)
{
    if(!sendDelay(Scope, delayus))
        return -1;

    readAll();
    return(d_currentScopeDelay);
}

void PinDelayGeneratorPA::readAll()
{
    QByteArray resp = p_comm->queryCmd(QString("S")).trimmed();// 'S' = get status, returns S:Delay A =%d, Delay B =%d\n\r
    if(resp.isEmpty() || !resp.startsWith("S:"))
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read state."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
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
        emit logMessage(QString("Could not parse response."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return;
    }

    emit protectionDelayUpdate(monitorDelay);
    emit scopeTriggerDelayUpdate(scopeDelay);

    d_currentProtectionDelay = monitorDelay;
    d_currentScopeDelay = scopeDelay;
}

bool PinDelayGeneratorPA::sendDelay(PinSwitchDriveDelayGenerator::Channel ch, int delayus)
{
    QByteArray resp;
    if(ch == Protection)
        resp = p_comm->queryCmd(QString("A%1\n").arg(delayus));
    else
        resp = p_comm->queryCmd(QString("B%1\n").arg(delayus));

    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not set delay."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return false;
    }

    return true;
}
