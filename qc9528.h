#ifndef QC9528_H
#define QC9528_H

#include "pulsegenerator.h"

class Qc9528 : public PulseGenerator
{
    Q_OBJECT
public:
    Qc9528(QObject *parent = nullptr);

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();
    void sleep(bool b);

    // PulseGenerator interface
public slots:
    QVariant read(const int index, const QtFTM::PulseSetting s);
    double readRepRate();
    bool set(const int index, const QtFTM::PulseSetting s, const QVariant val);
    bool setRepRate(double d);

private:
    bool pGenWriteCmd(QString cmd);
    bool d_forceExtClock;
};

#endif // QC9528_H
