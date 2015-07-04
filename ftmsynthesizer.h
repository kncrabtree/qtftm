#ifndef FTMSYNTHESIZER_H
#define FTMSYNTHESIZER_H

#include "synthesizer.h"

class FtmSynthesizer : public Synthesizer
{
	Q_OBJECT
public:
    explicit FtmSynthesizer(QObject *parent = nullptr);
	
signals:
    void newCavityFreq(double);
	void newProbeFreq(double);
	
public slots:
    virtual void initialize();
	void setCavityFreqFromUI(double d);
    bool setCavityFreq(double d);
	double goToProbeFreq();
	bool goToCavityFreq();
    void goToCavityDeltaFreq(double delta);

protected:
    double d_targetCavityFreq;
    double d_mixerOffset;
    double d_probeOffset;
	
};

#ifdef QTFTM_FTMSYNTH
#if QTFTM_FTMSYNTH == 1
#include "hp8673ftm.h"
class HP8673FTM;
typedef HP8673FTM FtmSynthHardware;
#elif QTFTM_FTMSYNTH == 2
#include "hp8340ftm.h"
class HP8340FTM;
typedef HP8340FTM FtmSynthHardware;
#else
#include "virtualftmsynth.h"
class VirtualFtmSynth;
typedef VirtualFtmSynth FtmSynthHardware;
#endif
#endif

#endif // FTMSYNTHESIZER_H
