#ifndef IOBOARD_H
#define IOBOARD_H
#include "hardwareobject.h"

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
    virtual long setCwMode(bool cw) =0;
    virtual void ftmSynthBand(int band) =0;
    virtual long setMagnet(bool mag) =0;
    virtual void checkForTrigger() =0;

protected:
    QTimer *p_readTimer;
    
};

#ifdef QTFTM_IOBOARD
#if QTFTM_IOBOARD == 1
#include "labjacku3.h"
class LabjackU3;
typedef LabjackU3 IOBoardHardware;
#else
#include "virtualioboard.h"
class VirtualIOBoard;
typedef VirtualIOBoard IOBoardHardware;
#endif
#endif

#endif // IOBOARD_H
