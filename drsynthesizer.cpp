#include "drsynthesizer.h"
#include <math.h>

DrSynthesizer::DrSynthesizer(QTcpSocket *s, QObject *parent) :
    Synthesizer(QString("drSynth"), QString("DR Synthesizer"), s,parent), d_freq(0.0), d_power(0.0)
{
    d_hardwareMinFreq = 50.0;
#ifdef HP8340
    d_hardwareMaxFreq = 26500.0;
#else
    d_hardwareMaxFreq = 26000.0;
#endif
    QSettings set(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    set.setValue(QString(d_key).append("/minPower"),-20.0);
    set.setValue(QString(d_key).append("/maxPower"),17.0);
    set.sync();
}

bool DrSynthesizer::testConnection()
{
    if(!GpibInstrument::testConnection())
    {
        emit connected(false,QString("GPIB error"));
        return false;
    }

    if(d_socket->bytesAvailable())
        d_socket->readAll();

#ifdef HP8340
    QByteArray resp = queryCmd(QString("OPCW;\n"));
#else
    QByteArray resp = queryCmd(QString("CFOA;\n")); // or maybe it's OACF ??
#endif
    if(resp.isEmpty())
    {
        emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
        return false;
    }
#ifdef HP8340
    if(!resp.startsWith("+"))
#else
    if(!resp.startsWith("CF"))
#endif
    {
        emit connected(false,QString("%1 response invalid. Received: %2").arg(d_prettyName).arg(QString(resp)));
        return false;
    }

    emit logMessage(QString("%1 ID response: %2").arg(d_prettyName).arg(QString(resp)));
    writeCmd(QString("RF1;\n"));    // This is the RF "ON" command, the same for both synths

    readFreq();
    readPower();

    emit connected();
    return true;
}

double DrSynthesizer::setFreq(double f)
{
    if(!calculateBand(f))
        return -1.0;

    //do communication here
    double rawFreq = realToRaw(f);
    if(rawFreq < 0.0)
        return -1.0;
#ifdef HP8340
    writeCmd(QString("CW%1MZ;\n").arg(QString::number(rawFreq,'f',3)));  // April 8 2015, PRAA - increase resolution
#else
    writeCmd(QString("CF%1MZ;\n").arg(QString::number(rawFreq,'f',3)));
#endif



	return readFreq();
}

double DrSynthesizer::readFreq()
{

#ifdef HP8340
    QByteArray resp = queryCmd(QString("OPCW;\n"));
#else
    QByteArray resp = queryCmd(QString("CFOA;\n"));
#endif
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 gave a null response to frequency query").arg(d_prettyName),LogHandler::Error);
        return -1.0;
    }
#ifdef HP8340
    if(!resp.startsWith("+"))
#else
    if(!resp.startsWith("CF"))
#endif
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse frequency response from %1. Response: %2 (Hex: %3)").arg(d_prettyName).arg(QString(resp)).arg(QString(resp.toHex())),LogHandler::Error);
        return -1.0;
    }

    //resp = resp.right(resp.length() -2); These needed for the non-8430 synth
    //resp = resp.left(resp.length() -3);
    double d = resp.trimmed().toDouble()/1.0e6;
    d_freq = rawToReal(d);

    emit newFreq(d_freq);
    return d_freq;
}

double DrSynthesizer::setPower(double p)
{
    if(fabs(p-d_power) > 0.09)
    {
        d_socket->waitForReadyRead(50);
        writeCmd(QString("PL%1DB;\n").arg(QString::number(p,'f',2)));
        d_socket->waitForReadyRead(50);// seems to be same command for both generators
        return readPower();
    }

    return d_power;
}

double DrSynthesizer::readPower()
{
    //first, determine if synth is leveled
//    QByteArray statusResp = queryCmd(QString("OS;\n"));
//    emit logMessage(QString("Status response hex: %1").arg(QString(statusResp.toHex())));

//    if(!statusResp.length() == 2)
//    {
//        emit hardwareFailure();
//        emit logMessage(QString("Could not read status bytes from %1. Response hex: %2").arg(d_prettyName).arg(QString(statusResp.toHex())),LogHandler::Error);
//        return -2e100;
//    }

    QByteArray resp;
//    if(statusResp.at(1) & 0x40) //if bit 6 is set, then the synth is unleveled, and we need to measure the power level
//    {
        //wait for power level to settle
//        d_socket->waitForReadyRead(500);
//        resp = queryCmd(QString("OR;\n"));
//    }
//    else //can just query the setpoint, because the actual value is within 0.1 dBm

#ifdef HP8340
        resp = queryCmd(QString("OPPL;\n"));
#else
        resp = queryCmd(QString("PLOA;\n"));
#endif
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 gave a null response to power query").arg(d_prettyName),LogHandler::Error);
        return -2e100;
    }

    bool ok = false;
    double p;
#ifdef HP8340
    p = resp.mid(1).trimmed().toDouble(&ok);
    if(resp.startsWith("-"))                    // I would think.toDouble would handle this ?? praa nov 2014
        p*=-1.0;
#else
    resp = resp.mid(2);                         // skip over the Flag bytes at the beginning "LE"
    resp = resp.left((resp.length() - 3));      // remove the units "DB" or "DM" at the end plus the end-of-line
    p = resp.toDouble(&ok);
#endif

    if(!ok)// && resp.length() == 2)
    {
//        if(statusResp.at(1) & 0x40) //if bit 6 is set, then the synth is unleveled, and we need to measure the power level
//        {
            //wait for power level to settle
//            d_socket->waitForReadyRead(500);
//            resp = queryCmd(QString(""));
//        }
//        else //can just query the setpoint, because the actual value is within 0.1 dBm
#ifdef HP8340
        resp = queryCmd(QString("OPPL;\n"));
#else
        resp = queryCmd(QString("PLOA;\n"));
#endif
        if(resp.isEmpty())
        {
            emit hardwareFailure();
            emit logMessage(QString("%1 gave a null response to power query").arg(d_prettyName),LogHandler::Error);
            return -2e100;
        }

        ok = false;
        p = resp.mid(1).trimmed().toDouble(&ok);
        //if(resp.startsWith("-"))
          //  p*=-1.0;

        if(!ok)
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not parse power response from %1. Response: %2 (Hex: %3)").arg(d_prettyName).arg(QString(resp)).arg(QString(resp.toHex())),LogHandler::Error);
            return -2e100;
        }
    }
//    else if(!ok)
//    {
//        emit hardwareFailure();
//        emit logMessage(QString("Could not parse power response from %1. Response: %2 (Hex: %3)").arg(d_prettyName).arg(QString(resp)).arg(QString(resp.toHex())),LogHandler::Error);
//        return -2e100;
//    }

    d_power = p;
    emit newPower(d_power);
    return d_power;
}
