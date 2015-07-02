#ifndef LABJACKU3_H
#define LABJACKU3_H

#include "ioboard.h"

#include "u3.h"

class LabjackU3 : public IOBoard
{
    Q_OBJECT
public:
    explicit LabjackU3(QObject *parent = nullptr);

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // IOBoard interface
public slots:
    long setCwMode(bool cw);
    void ftmSynthBand(int band);
    long setMagnet(bool mag);
    void checkForTrigger();

private:
    HANDLE d_handle;
    u3CalibrationInfo d_calInfo;

    int d_serialNo;
    int d_cwLine;
    int d_highBandLine;
    int d_magnetLine;
    int d_counterPinOffset;
    bool d_blockTriggering;
    quint32 d_counterCount;

    void configure();
    void resetCounter();
    quint32 readCounter();
    void closeConnection();

};

#endif // LABJACKU3_H
