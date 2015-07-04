#include "virtualdrsynth.h"

#include "virtualinstrument.h"

VirtualDrSynth::VirtualDrSynth(QObject *parent) :
	DrSynthesizer(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual DR Synthesizer");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);

	d_currentSynthFreq = 10000.0;
	d_currentSynthPower = 17.0;

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

VirtualDrSynth::~VirtualDrSynth()
{

}



bool VirtualDrSynth::testConnection()
{
	readFreq();
	readPower();

	emit connected();
	return true;
}

void VirtualDrSynth::initialize()
{
	DrSynthesizer::initialize();
	testConnection();
}

double VirtualDrSynth::setSynthFreq(double d)
{
	d_currentSynthFreq = d;
	return readFreq();
}

double VirtualDrSynth::readSynthFreq()
{
	return d_currentSynthFreq;
}

double VirtualDrSynth::setSynthPower(double d)
{
	d_currentSynthPower = d;
	return readPower();
}

double VirtualDrSynth::readSynthPower()
{
	return d_currentSynthPower;
}
