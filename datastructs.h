#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#include <QString>
#include <QMetaType>

namespace QtFTM {

enum LogMessageCode {
    LogNormal,
    LogWarning,
    LogError,
    LogHighlight
};

enum PulseActiveLevel {
    PulseLevelActiveLow,
    PulseLevelActiveHigh
};

enum PulseSetting {
    PulseDelay,
    PulseWidth,
    PulseEnabled,
    PulseLevel,
    PulseName
};

enum ScopeResolution {
    Res_1kHz,
    Res_2kHz,
    Res_5kHz,
    Res_10kHz
};

enum FlowSetting {
    FlowSettingEnabled,
    FlowSettingSetpoint,
    FlowSettingFlow,
    FlowSettingName
};

struct FlowChannelConfig {
    bool enabled;
    double setpoint;
    QString name;
};

struct PulseChannelConfig {
    int channel;
    QString channelName;
    bool enabled;
    double delay;
    double width;
    PulseActiveLevel level;

    PulseChannelConfig() : channel(-1), enabled(false), delay(-1.0), width(-1.0), level(PulseLevelActiveHigh) {}
};

}

Q_DECLARE_METATYPE(QtFTM::FlowSetting)
Q_DECLARE_METATYPE(QtFTM::PulseActiveLevel)
Q_DECLARE_METATYPE(QtFTM::LogMessageCode)
Q_DECLARE_METATYPE(QtFTM::ScopeResolution)

#endif // DATASTRUCTS_H

