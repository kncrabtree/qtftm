#include "ftmsynthesizer.h"
#include <QApplication>
#include <math.h>

FtmSynthesizer::FtmSynthesizer(QObject *parent) :
    Synthesizer(parent),
    d_targetCavityFreq(0.0), d_mixerOffset(30.0), d_probeOffset(0.4)
{
    d_key = QString("ftmSynth");
}

FtmSynthesizer::~FtmSynthesizer()
{

}

void FtmSynthesizer::initialize()
{
    Synthesizer::initialize();

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);

    d_mixerOffset = s.value(QString("mixerOffset"),30.0).toDouble();
    d_probeOffset = s.value(QString("probeOffset"),0.4).toDouble();
    s.setValue(QString("mixerOffset"),d_mixerOffset);
    s.setValue(QString("probeOffset"),d_probeOffset);

    s.endGroup();
    s.endGroup();
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
        emit newCavityFreq(d_targetCavityFreq);
        return false;
    }

	//this sets the target cavity frequency and updates the offsets if they've changed. This does NOT change the synthesizer frequency!
	d_targetCavityFreq = d;

	//show cavity freq on UI control box
    emit newCavityFreq(d_targetCavityFreq);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    d_mixerOffset = s.value(QString("%1/%2/mixerOffset").arg(d_key).arg(d_subKey),30.0).toDouble();
    d_probeOffset = s.value(QString("%1/%2/probeOffset").arg(d_key).arg(d_subKey),0.4).toDouble();

    return true;
}

bool FtmSynthesizer::goToCavityFreq()
{
    bool success = setFreqForce(d_targetCavityFreq-d_mixerOffset*d_mult);

	if(!success)
		emit hardwareFailure();

    double f = readFreq();
    emit newProbeFreq(f+d_mixerOffset*d_mult);

    return success;
}

double FtmSynthesizer::goToProbeFreq()
{
    bool success = setFreqForce(d_targetCavityFreq-d_probeOffset-d_mixerOffset*d_mult);
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

void FtmSynthesizer::goToCavityDeltaFreq(double delta)
{
    setFreqForce(d_targetCavityFreq-d_mixerOffset*d_mult+delta);

    //make sure frequency gets updated before returning
    double f = readFreq();
    emit newProbeFreq(f+d_mixerOffset*d_mult);
}
