#include "ftmsynthesizer.h"
#include <QApplication>
#include <math.h>

FtmSynthesizer::FtmSynthesizer(QTcpSocket *s, QObject *parent) :
    Synthesizer(QString("ftmSynth"),QString("FTM Synthesizer"),s,parent),
    d_targetCavityFreq(0.0), d_mixerOffset(30.0), d_probeOffset(0.4)
{
    d_hardwareMinFreq = 100;
    d_hardwareMaxFreq = 26490;
}


bool FtmSynthesizer::testConnection()
{
    if(!GpibInstrument::testConnection())
    {
        emit connected(false,QString("GPIB error."));
        return false;
    }

    QByteArray resp = queryCmd(QString("CFOA;\n"));
//    QByteArray resp = queryCmd(QString("OPCW;\n"));

    if(resp.isEmpty())
    {
        emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
        return false;
    }
    if(!resp.startsWith("CF"))
//    if(!resp.startsWith("+"))
    {
        emit connected(false,QString("%1 response invalid. Received: %2").arg(d_prettyName).arg(QString(resp)));
        return false;
    }
    emit logMessage(QString("%1 ID response: %2").arg(d_prettyName).arg(QString(resp)));

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    d_mixerOffset = s.value(key().append(QString("/mixerOffset")),30.0).toDouble();
    d_probeOffset = s.value(key().append(QString("/probeOffset")),0.4).toDouble();


    d_targetCavityFreq = rawToReal(resp.mid(2,11).toDouble()/1.0e6) + d_mixerOffset*d_mult + d_probeOffset;
    emit logMessage(QString("Initial cavity frequency is %1 MHz").arg(d_targetCavityFreq));
    emit newFreq(d_targetCavityFreq);
    emit newProbeFreq(d_targetCavityFreq-d_probeOffset);
    emit connected();
    return true;
}

void FtmSynthesizer::setCavityFreqFromUI(double d)
{
	//note: this is called from UI. Need to update the target frequency, then go to the probe frequency
	setCavityFreq(d);
	goToProbeFreq();
}

bool FtmSynthesizer::setCavityFreq(double d)
{
    if(!calculateBand(d))
    {
        emit newFreq(d_targetCavityFreq);
        return false;
    }

	//this sets the target cavity frequency and updates the offsets if they've changed. This does NOT change the synthesizer frequency!
	d_targetCavityFreq = d;


	//show cavity freq on UI control box
	emit newFreq(d_targetCavityFreq);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	d_mixerOffset = s.value(key().append(QString("/mixerOffset")),30.0).toDouble();
	d_probeOffset = s.value(key().append(QString("/probeOffset")),0.4).toDouble();

    return true;
}

bool FtmSynthesizer::goToCavityFreq()
{
    bool success = setSynthFreq(d_targetCavityFreq-d_mixerOffset*d_mult);

	if(!success)
		emit hardwareFailure();

    double f = readFreq();
    emit newProbeFreq(f+d_mixerOffset*d_mult);

    return success;
}

double FtmSynthesizer::goToProbeFreq()
{
    bool success = setSynthFreq(d_targetCavityFreq-d_probeOffset-d_mixerOffset*d_mult);
	if(!success)
	{
		emit hardwareFailure();
		return -1.0;
	}

	double f = readFreq();
	if(f < 0.0)
	{
		emit hardwareFailure();
		return f;
	}



	//return the probe frequency
    emit newProbeFreq(f+d_mixerOffset*d_mult);
    return f+d_mixerOffset*d_mult;
}

double FtmSynthesizer::readFreq()
{
    QByteArray resp = queryCmd(QString("CFOA;\n"));
    if(!resp.startsWith("CF"))
    {
        emit hardwareFailure();
        return -1.0;
    }

    return rawToReal(resp.mid(2,11).toDouble()/1.0e6);

//    QByteArray resp = queryCmd(QString("OPCW;\n"));
//    if(!resp.startsWith("+"))
//    {
//        emit hardwareFailure();
//        return -1.0;
//    }

//    return rawToReal(resp.mid(1).trimmed().toDouble()/1.0e6);
}

void FtmSynthesizer::goToCavityDeltaFreq(double delta)
{
    setSynthFreq(d_targetCavityFreq-d_mixerOffset*d_mult+delta);

    //make sure frequency gets updated before returning
    double f = readFreq();
    emit newProbeFreq(f+d_mixerOffset*d_mult);
}

bool FtmSynthesizer::setSynthFreq(double d)
{
    double rawFreq = realToRaw(d);

    if(rawFreq<50.0 || rawFreq>26500.0)
        return false;

    //Figure out the HP8673D band used to generate the frequency, and select nearest valid value
    //See manual for details
    int hpBand = 1;
    if(rawFreq>6600.0)
        hpBand++;
    if(rawFreq>12300.0)
        hpBand++;
    if(rawFreq>18600.0)
        hpBand++;

    int freqkHz = (int)round(rawFreq*1000.0);
    if(freqkHz % hpBand)
    {
        //this means that the target frequency cannot be set exactly. select nearest valid frequency
        rawFreq = round(freqkHz/(double)hpBand)/1000.0*(double)hpBand;
    }

    if(rawFreq<50.0 || rawFreq>26500.0)
        return false;

	//write command to synth here!
    if (!writeCmd(QString("CF%1MZ;\n").arg(rawFreq,0,'f',3)))
//    if (!writeCmd(QString("CW%1MZ;\n").arg(rawFreq,0,'f',3)))
    {
        emit hardwareFailure();
        return false;
    }

    return true;
}
