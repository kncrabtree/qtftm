#ifndef PULSEGENERATOR_H
#define PULSEGENERATOR_H

#include "hardwareobject.h"

#include "pulsegenconfig.h"

class PulseGenerator : public HardwareObject
{
    Q_OBJECT
public:
    PulseGenerator(QObject *parent = nullptr);
    ~PulseGenerator();

    struct PulseChannelConfiguration {
        int channel;
        QString channelName;
        bool enabled;
        double delay;
        double width;
//        ActiveLevel active;

//        PulseChannelConfiguration()
//            : channel(-1), channelName(QString()), enabled(false), delay(0.0), width(0.0), active(ActiveHigh) {}
//        explicit PulseChannelConfiguration(int ch, QString n, bool en, double d, double w, ActiveLevel a)
//            : channel(ch), channelName(n), enabled(en), delay(d), width(w), active(a) {}

    };

public slots:
    PulseGenConfig config() const { return d_config; }
    virtual QVariant read(const int index, const QtFTM::PulseSetting s) =0;
    virtual double readRepRate() =0;

    virtual QtFTM::PulseChannelConfig read(const int index);

    virtual bool set(const int index, const QtFTM::PulseSetting s, const QVariant val) =0;
    virtual bool setChannel(const int index, const QtFTM::PulseChannelConfig cc);
    virtual bool setAll(const PulseGenConfig cc);

    virtual bool setRepRate(double d) =0;

signals:
    void settingUpdate(int,QtFTM::PulseSetting,QVariant);
    void configUpdate(const PulseGenConfig);
    void repRateUpdate(double);

protected:
    PulseGenConfig d_config;
    virtual void readAll();
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
