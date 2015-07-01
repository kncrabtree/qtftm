#ifndef PINSWITCHDRIVEDELAYGENERATOR_H
#define PINSWITCHDRIVEDELAYGENERATOR_H


#include "rs232instrument.h"
#include <QPair>
#include <QDateTime>


class PinSwitchDriveDelayGenerator : public Rs232Instrument
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
    void initialize();
    bool testConnection();

    int setProtectionDelay(int delayInMicroseconds);
    int setScopeDelay(int delayInMicroscconds);

    void readAll();

     int currentScopeDelay() const;

     int currentProtectionDelay() const;




    void shutUp(bool quiet) { d_quiet = quiet; }

private:

    int d_currentScopeDelay, d_currentProtectionDelay, d_quiet;



    bool sendDelay(Channel ch, int delayInMicroSeconds);
};


#endif // PINSWITCHDRIVEDELAYGENERATOR_H
