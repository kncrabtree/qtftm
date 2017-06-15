#ifndef N5813FTM_H
#define N5813FTM_H

#include "ftmsynthesizer.h"


class N5813FTM : public FtmSynthesizer
{
public:
    N5813FTM();

    // HardwareObject interface
public slots:
    virtual bool testConnection();
    virtual void initialize();

    // Synthesizer interface
protected:
    virtual double setSynthFreq(double d);
    virtual double readSynthFreq();
    virtual double setSynthPower(double d);
    virtual double readSynthPower();
};

#endif // N5813FTM_H
