#ifndef PINSWITCHDRIVEDELAYGENERATOR_H
#define PINSWITCHDRIVEDELAYGENERATOR_H

#include "hardwareobject.h"

class PinSwitchDriveDelayGenerator : public HardwareObject
{
    Q_OBJECT
public:
    enum Channel {
        Protection,
        Scope
    };

    explicit PinSwitchDriveDelayGenerator(QObject *parent = nullptr);


signals:
    void protectionDelayUpdate(int);
    void scopeTriggerDelayUpdate(int);

public slots:
    virtual int setProtectionDelay(int delayInMicroseconds) =0;
    virtual int setScopeDelay(int delayInMicroscconds) =0;
    virtual void readAll() =0;

protected:
    int d_currentScopeDelay;
    int d_currentProtectionDelay;

};

#ifdef QTFTM_PDG
#if QTFTM_PDG == 1
#include "pindelaygeneratorpa.h"
class PinDelayGeneratorPA;
typedef PinDelayGeneratorPA PDGHardware;
#else
#include "virtualpindelaygenerator"
class VirtualPinDelayGenerator;
typedef VirtualPinDelayGenerator PDGHardware
#endif
#endif

#endif // PINSWITCHDRIVEDELAYGENERATOR_H
