#include "hp8673dr.h"

HP8673DR::HP8673DR(QObject *parent) :
    DrSynthesizer(parent)
{
    d_subKey = QString("hp8673");
    d_prettyName = QString("HP8873 DR Synthesizer");
    d_commType = CommunicationProtocol::Gpib;

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



bool HP8673DR::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    QByteArray resp = p_comm->queryCmd(QString("CFOA;\n"));
    if(resp.isEmpty())
    {
        emit connected(false,QString("Null response to ID query"));
        return false;
    }
    if(!resp.startsWith("CF"))
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

void HP8673DR::initialize()
{
    DrSynthesizer::initialize();

    p_comm->initialize();
    testConnection();
}

double HP8673DR::setSynthFreq(double d)
{
    //Figure out the HP8673D band used to generate the frequency, and select nearest valid value
    //See manual for details
    int hpBand = 1;
    if(d>6600.0)
        hpBand++;
    if(d>12300.0)
        hpBand++;
    if(d>18600.0)
        hpBand++;

    int freqkHz = qRound(d*1000.0);
    double rawFreq = freqkHz/1000.0;
    if(freqkHz % hpBand)
    {
        //this means that the target frequency cannot be set exactly. select nearest valid frequency
        rawFreq = round((double)freqkHz/(double)hpBand)/1000.0*(double)hpBand;
    }

    //write command to synth here!
    if (!p_comm->writeCmd(QString("CF%1MZ;\n").arg(rawFreq,0,'f',3)))
    {
        emit hardwareFailure();
        return -1.0;
    }

    return readFreq();
}

double HP8673DR::readSynthFreq()
{
    QByteArray resp = p_comm->queryCmd(QString("CFOA;\n"));
    if(!resp.startsWith("CF"))
    {
        emit hardwareFailure();
        return -1.0;
    }

    return resp.mid(2,11).trimmed().toDouble()/1.0e6;
}

double HP8673DR::setSynthPower(double p)
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

double HP8673DR::readSynthPower()
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
