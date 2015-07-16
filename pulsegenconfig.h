#ifndef PULSEGENCONFIG_H
#define PULSEGENCONFIG_H

#include <QSharedDataPointer>

#include <QList>
#include <QVariant>
#include <QMap>

#include "datastructs.h"


class PulseGenConfigData;

class PulseGenConfig
{
public:
    PulseGenConfig();
    PulseGenConfig(const PulseGenConfig &);
    PulseGenConfig &operator=(const PulseGenConfig &);
    ~PulseGenConfig();

    QtFTM::PulseChannelConfig at(const int i) const;
    int size() const;
    bool isEmpty() const;
    QVariant setting(const int index, const QtFTM::PulseSetting s) const;
    QtFTM::PulseChannelConfig settings(const int index) const;
    double repRate() const;
    QString headerString() const;

    void set(const int index, const QtFTM::PulseSetting s, const QVariant val);
    void set(const int index, const QtFTM::PulseChannelConfig cc);
    void add(const QString name, const bool enabled, const double delay, const double width, const QtFTM::PulseActiveLevel level);
    void setRepRate(const double r);

    void setDcEnabled(bool en);
    void setDrEnabled(bool en);
    bool isDcEnabled() const;
    bool isDrEnabled() const;

private:
    QSharedDataPointer<PulseGenConfigData> data;
};

class PulseGenConfigData : public QSharedData
{
public:
    QList<QtFTM::PulseChannelConfig> config;
    double repRate;
};

#endif // PULSEGENCONFIG_H
