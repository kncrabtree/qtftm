#ifndef IOBOARD_H
#define IOBOARD_H
#include "hardwareobject.h"
#include "u3.h"
#include <QTimer>

class IOBoard : public HardwareObject
{
    Q_OBJECT
public:
    explicit IOBoard(QObject *parent = nullptr);
    
signals:
    void triggered();
    void magnetUpdate(bool);
    
public slots:
    void initialize();
    bool testConnection();
    void configure();

    long cwMode(bool cw);
    void ftmSynthBand(int band);
    long setMagnet(bool mag);
    void checkCounter();

private:
    HANDLE d_handle;
    u3CalibrationInfo d_calInfo;

    void resetCounter();
    quint32 readCounter();
    void closeConnection();
    quint32 d_counterCount;
    QTimer *counterTimer;

    bool d_blockTriggering;

    int d_serialNo;
    int d_cwLine;
    int d_highBandLine;
    int d_magnetLine;
    int d_counterPinOffset;
    
};

#endif // IOBOARD_H
