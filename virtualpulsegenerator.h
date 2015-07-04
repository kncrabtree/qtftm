#ifndef VIRTUALPULSEGENERATOR_H
#define VIRTUALPULSEGENERATOR_H

#include "pulsegenerator.h"

class VirtualPulseGenerator : public PulseGenerator
{
    Q_OBJECT
public:
    explicit VirtualPulseGenerator(QObject *parent = nullptr);
    ~VirtualPulseGenerator();

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // PulseGenerator interface
public slots:
    QVariant read(const int index, const QtFTM::PulseSetting s);
    double readRepRate();

    bool set(const int index, const QtFTM::PulseSetting s, const QVariant val);
    bool setRepRate(double d);
};

#endif // VIRTUALPULSEGENERATOR_H
