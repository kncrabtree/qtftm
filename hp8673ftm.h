#ifndef HP8673FTM_H
#define HP8673FTM_H

#include "ftmsynthesizer.h"

class HP8673FTM : public FtmSynthesizer
{
public:
    explicit HP8673FTM(QObject *parent = nullptr);

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // Synthesizer interface
protected:
    double setSynthFreq(double d);
    double readSynthFreq();
    double setSynthPower(double p);
    double readSynthPower();
};

#endif // HP8673FTM_H
