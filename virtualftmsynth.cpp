#include "virtualftmsynth.h"

#include "virtualinstrument.h"

VirtualFtmSynth::VirtualFtmSynth(QObject *parent) :
	FtmSynthesizer(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual FTM Synthesizer");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(d_key);
	s.beginGroup(d_subKey);
    d_hardwareMinFreq = s.value(QString("hardwareMinFreq"),50.0).toDouble();
	d_hardwareMaxFreq = s.value(QString("hardwareMaxFreq"),26490.0).toDouble();

	s.setValue(QString("hardwareMinFreq"),d_hardwareMinFreq);
	s.setValue(QString("hardwareMaxFreq"),d_hardwareMaxFreq);
	s.endGroup();
	s.endGroup();

	d_currentSynthFreq = 4969.6;
	d_currentSynthPower = 10.0;
}

VirtualFtmSynth::~VirtualFtmSynth()
{

}



bool VirtualFtmSynth::testConnection()
{
	d_targetCavityFreq = rawToReal(d_currentSynthFreq) + d_mixerOffset*d_mult + d_probeOffset;
	emit newCavityFreq(d_targetCavityFreq);
	emit newProbeFreq(d_targetCavityFreq-d_probeOffset);

	emit connected();
	return true;
}

void VirtualFtmSynth::initialize()
{
	FtmSynthesizer::initialize();
	testConnection();
}

double VirtualFtmSynth::setSynthFreq(double d)
{
	d_currentSynthFreq = d;
	return readFreq();
}

double VirtualFtmSynth::readSynthFreq()
{
	return d_currentSynthFreq;
}

double VirtualFtmSynth::setSynthPower(double d)
{
	d_currentSynthPower = d;
	return readPower();
}

double VirtualFtmSynth::readSynthPower()
{
	return d_currentSynthPower;
}
