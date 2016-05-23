#include "n5183dr.h"

#include "gpibinstrument.h"

N5183Dr::N5183Dr(QObject *parent) : DrSynthesizer(parent)
{
    d_subKey = QString("n5183");
    d_prettyName = QString("Keysight N5183 DR Synthesizer");

    p_comm = new GpibInstrument(d_key,d_subKey,static_cast<GpibController*>(parent),this);
    connect(p_comm,&CommunicationProtocol::logMessage,this,&N5183Dr::logMessage);
    connect(p_comm,&CommunicationProtocol::hardwareFailure,this,&N5183Dr::hardwareFailure);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    d_hardwareMinFreq = s.value(QString("hardwareMinFreq"),100.0).toDouble();
    d_hardwareMaxFreq = s.value(QString("hardwareMaxFreq"),40000.0).toDouble();
    d_hardwareMinPower = s.value(QString("minPower"),-20.0).toDouble();
    d_hardwareMaxPower = s.value(QString("maxPower"),17.0).toDouble();

    s.setValue(QString("minPower"),d_hardwareMinPower);
    s.setValue(QString("maxPower"),d_hardwareMaxPower);
    s.setValue(QString("hardwareMinFreq"),d_hardwareMinFreq);
    s.setValue(QString("hardwareMaxFreq"),d_hardwareMaxFreq);
    s.endGroup();
    s.endGroup();
}



bool N5183Dr::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }


    QByteArray resp = p_comm->queryCmd(QString("*IDN\n"));
    if(resp.isEmpty())
    {
        emit connected(false,QString("Null response to ID query"));
        return false;
    }
    if(!resp.contains("N5183"))
    {
        emit connected(false,QString("ID response invalid. Received: %1").arg(QString(resp)));
        return false;
    }
    emit logMessage(QString("ID response: %1").arg(QString(resp)));

    readFreq();
    readPower();

    emit connected();
    return true;
}

void N5183Dr::initialize()
{
    DrSynthesizer::initialize();

    p_comm->initialize();
    testConnection();
}

double N5183Dr::setSynthFreq(double d)
{
    if (!p_comm->writeCmd(QString(":FREQ %1MHZ\n").arg(d,0,'f',3)))
    {
        emit hardwareFailure();
        return -1.0;
    }

    return readFreq();
}

double N5183Dr::readSynthFreq()
{
    QByteArray resp = p_comm->queryCmd(QString(":FREQ?\n"));

    return resp.trimmed().toDouble()/1.0e6;
}

double N5183Dr::setSynthPower(double d)
{
    double power = readSynthPower();
    if(qAbs(d-power) > 0.09)
    {
        p_comm->device()->waitForReadyRead(50);
        p_comm->writeCmd(QString(":power %1DB\n").arg(QString::number(d,'f',2)));
        p_comm->device()->waitForReadyRead(50);// seems to be same command for both generators
    }

    return readPower();
}

double N5183Dr::readSynthPower()
{
    QByteArray resp = p_comm->queryCmd(QString(":POWER?\n"));

    bool ok = false;
    double p;
    p = resp.trimmed().toDouble(&ok);

    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("Null response to power query"),QtFTM::LogError);
        return -2e100;
    }

    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse power response. Response: %1 (Hex: %2)")
                        .arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return -2e100;
    }

    return p;
}
