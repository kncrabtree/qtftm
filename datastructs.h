#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#include <QString>

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

}

#endif // DATASTRUCTS_H

