#ifndef HP8340DR_H
#define HP8340DR_H

#include "drsynthesizer.h"

class HP8340DR : public DrSynthesizer
{
public:
    explicit HP8340DR(QObject *parent = nullptr);

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

#endif // HP8340DR_H
