#include "hp8340dr.h"

#include "gpibinstrument.h"

HP8340DR::HP8340DR(QObject *parent) :
    DrSynthesizer(parent)
{
    d_subKey = QString("hp8340");
    d_prettyName = QString("HP8340 DR Synthesizer");

    p_comm = new GpibInstrument(d_key,d_subKey,static_cast<GpibController*>(parent),this);
    connect(p_comm,&CommunicationProtocol::logMessage,this,&HP8340DR::logMessage);
    connect(p_comm,&CommunicationProtocol::hardwareFailure,this,&HP8340DR::hardwareFailure);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    d_hardwareMinFreq = s.value(QString("hardwareMinFreq"),100.0).toDouble();
    d_hardwareMaxFreq = s.value(QString("hardwareMaxFreq"),26490.0).toDouble();
    d_hardwareMinPower = s.value(QString("minPower"),-20.0).toDouble();
    d_hardwareMaxPower = s.value(QString("maxPower"),17.0).toDouble();

    s.setValue(QString("minPower"),d_hardwareMinPower);
    s.setValue(QString("maxPower"),d_hardwareMaxPower);
    s.setValue(QString("hardwareMinFreq"),d_hardwareMinFreq);
    s.setValue(QString("hardwareMaxFreq"),d_hardwareMaxFreq);
    s.endGroup();
    s.endGroup();
}



bool HP8340DR::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    QByteArray resp = p_comm->queryCmd(QString("OPCW;\n"));
    if(resp.isEmpty())
    {
        emit connected(false,QString("Null response to ID query"));
        return false;
    }
    if(!resp.startsWith("+"))
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

void HP8340DR::initialize()
{
    DrSynthesizer::initialize();

    p_comm->initialize();
    testConnection();
}

double HP8340DR::setSynthFreq(double d)
{
    if (!p_comm->writeCmd(QString("CW%1MZ;\n").arg(d,0,'f',3)))
    {
        emit logMessage(QString("Could not set synth frequency to %1.").arg(d),QtFTM::LogError);
        emit hardwareFailure();
        return -1.0;
    }

    return readFreq();
}

double HP8340DR::readSynthFreq()
{
    QByteArray resp = p_comm->queryCmd(QString("OPCW;\n"));
    if(!resp.startsWith("+"))
    {
        emit hardwareFailure();
        return -1.0;
    }

    return resp.mid(1).trimmed().toDouble()/1.0e6;
}

double HP8340DR::setSynthPower(double p)
{
    double power = readSynthPower();
    if(qAbs(p-power) > 0.09)
    {
        p_comm->device()->waitForReadyRead(50);
	   p_comm->writeCmd(QString("PL%1DB;\n").arg(QString::number(p,'f',2)));
        p_comm->device()->waitForReadyRead(50);// seems to be same command for both generators
    }

    return readPower();
}

double HP8340DR::readSynthPower()
{
    QByteArray resp = p_comm->queryCmd(QString("OPPL;\n"));

    bool ok = false;
    double p;
    p = resp.trimmed().toDouble(&ok);

    if(!ok)
    {
        resp = p_comm->queryCmd(QString("OPPL;\n"));

        if(resp.isEmpty())
        {
            emit hardwareFailure();
            emit logMessage(QString("Null response to power query"),QtFTM::LogError);
            return -2e100;
        }

        ok = false;
        p = resp.mid(1).trimmed().toDouble(&ok);

        if(!ok)
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not parse power response. Response: %1 (Hex: %2)")
                            .arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
            return -2e100;
        }
    }

    return p;
}
