#ifndef PINDELAYGENERATORPA_H
#define PINDELAYGENERATORPA_H

#include "pinswitchdrivedelaygenerator.h"

class PinDelayGeneratorPA : public PinSwitchDriveDelayGenerator
{
    Q_OBJECT
public:
    explicit PinDelayGeneratorPA(QObject *parent = nullptr);

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // PinSwitchDriveDelayGenerator interface
public slots:
    int setProtectionDelay(int delayus);
    int setScopeDelay(int delayus);
    void readAll();

private:
    bool sendDelay(Channel ch, int delayus);
};


#endif // PINDELAYGENERATORPA_H
