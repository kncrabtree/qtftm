#ifndef HP8673DR_H
#define HP8673DR_H

#include "drsynthesizer.h"

class HP8673DR : public DrSynthesizer
{
public:
    explicit HP8673DR(QObject *parent = nullptr);

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

#endif // HP8673DR_H
