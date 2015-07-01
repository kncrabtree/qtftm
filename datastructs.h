#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

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

}

#endif // DATASTRUCTS_H

