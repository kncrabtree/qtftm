#include "pinswitchdrivedelaygenerator.h"

#include <QStringList>

PinSwitchDriveDelayGenerator::PinSwitchDriveDelayGenerator (QObject *parent) :
    HardwareObject(parent), d_currentScopeDelay(5), d_currentProtectionDelay(5)
{
    d_key = QString("pdg");
}



