#ifndef HP8340FTM_H
#define HP8340FTM_H

#include "ftmsynthesizer.h"

class HP8340FTM : public FtmSynthesizer
{
public:
    explicit HP8340FTM(QObject *parent = nullptr);

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

#endif // HP8340FTM_H
