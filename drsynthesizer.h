#ifndef DRSYNTHESIZER_H
#define DRSYNTHESIZER_H

#include "synthesizer.h"

class DrSynthesizer : public Synthesizer
{
	Q_OBJECT
public:
    explicit DrSynthesizer(QObject *parent = nullptr);
	
public slots:
    virtual void initialize();
	
};

#ifdef QTFTM_DRSYNTH
#if QTFTM_DRSYNTH == 1
#include "hp8673dr.h"
class HP8673DR;
typedef HP8673DR DrSynthHardware;
#elif QTFTM_DRSYNTH == 2
#include "hp8340dr.h"
class HP8340DR;
typedef HP8340DR DrSynthHardware;
#else
#include "virtualdrsynth.h"
class VirtualDrSynth;
typedef VirtualDrSynth DrSynthHardware;
#endif
#endif

#endif // DRSYNTHESIZER_H
