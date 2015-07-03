#include "pulsegenerator.h"

PulseGenerator::PulseGenerator(QObject *parent) :
   HardwareObject(parent), d_minDelay(0.0), d_maxDelay(100000.0), d_minWidth(0.001), d_maxWidth(100000.0)
{
    d_key = QString("pulseGenerator");
}

PulseGenerator::~PulseGenerator()
{

}


QtFTM::PulseChannelConfig PulseGenerator::read(const int index)
{
    QtFTM::PulseChannelConfig out = d_config.settings(index);
    bool ok = false;
    out.channelName = read(index,QtFTM::PulseName).toString();
    out.delay = read(index,QtFTM::PulseDelay).toDouble(&ok);
    if(!ok)
        return out;
    out.width = read(index,QtFTM::PulseWidth).toDouble(&ok);
    if(!ok)
        return out;
    out.enabled = read(index,QtFTM::PulseEnabled).toBool();
    out.level = read(index,QtFTM::PulseLevel).value<QtFTM::PulseActiveLevel>();

    return out;
}


bool PulseGenerator::setChannel(const int index, const QtFTM::PulseChannelConfig cc)
{
    bool success = true;

    success &= set(index,QtFTM::PulseName,cc.channelName);
    success &= set(index,QtFTM::PulseEnabled,cc.enabled);
    success &= set(index,QtFTM::PulseDelay,cc.delay);
    success &= set(index,QtFTM::PulseWidth,cc.width);
    success &= set(index,QtFTM::PulseLevel,cc.level);

    return success;
}

bool PulseGenerator::setAll(const PulseGenConfig cc)
{
    bool success = true;
    for(int i=0; i<d_config.size(); i++)
        success &= setChannel(i,cc.at(i));

    success &= setRepRate(cc.repRate());

    return success;
}

PulseGenConfig PulseGenerator::configureForTuning()
{
	PulseGenConfig out = d_config;
	//disable everything except gas and MW
	for(int i=0; i<d_config.size(); i++)
	{
		if(i != QTFTM_PGEN_GASCHANNEL && i != QTFTM_PGEN_MWCHANNEL)
			set(i,QtFTM::PulseEnabled,false);
	}
	return out;
}

void PulseGenerator::readAll()
{
    for(int i=0;i<QTFTM_PGEN_NUMCHANNELS; i++)
        read(i);

    readRepRate();
}
