#ifndef PULSEGENERATOR_H
#define PULSEGENERATOR_H

#include "hardwareobject.h"

#include "pulsegenconfig.h"

class PulseGenerator : public HardwareObject
{
    Q_OBJECT
public:
    PulseGenerator(QObject *parent = nullptr);
    virtual ~PulseGenerator();

public slots:
    PulseGenConfig config() const { return d_config; }
    virtual QVariant read(const int index, const QtFTM::PulseSetting s) =0;
    virtual double readRepRate() =0;

    virtual QtFTM::PulseChannelConfig read(const int index);

    virtual bool set(const int index, const QtFTM::PulseSetting s, const QVariant val) =0;
    virtual bool setChannel(const int index, const QtFTM::PulseChannelConfig cc);
    virtual bool setAll(const PulseGenConfig cc);

    virtual bool setRepRate(double d) =0;

    PulseGenConfig configureForTuning();

signals:
    void settingUpdate(int,QtFTM::PulseSetting,QVariant);
    void configUpdate(const PulseGenConfig);
    void repRateUpdate(double);

protected:
    PulseGenConfig d_config;
    virtual void readAll();

    double d_minDelay;
    double d_maxDelay;
    double d_minWidth;
    double d_maxWidth;
};

#ifdef QTFTM_PGEN
#if QTFTM_PGEN==1
#include "qc9518.h"
class QC9518;
typedef QC9518 PulseGeneratorHardware;
#else
#include "virtualpulsegenerator.h"
class VirtualPulseGenerator;
typedef VirtualPulseGenerator PulseGeneratorHardware;
#endif
#endif

#endif // PULSEGENERATOR_H
