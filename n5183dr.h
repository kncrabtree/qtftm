#ifndef N5183DR_H
#define N5183DR_H

#include "drsynthesizer.h"

class N5183Dr : public DrSynthesizer
{
public:
    N5183Dr(QObject *parent = nullptr);

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // Synthesizer interface
protected:
    double setSynthFreq(double d);
    double readSynthFreq();
    double setSynthPower(double d);
    double readSynthPower();
};

#endif // N5183DR_H
