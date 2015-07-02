#ifndef MOTORDRIVER_H
#define MOTORDRIVER_H

#include "hardwareobject.h"
#include <QPair>
#include <QDateTime>
#include <QStringList>

#ifdef QTFTM_FTM1
#define MD_MIRRORROC 83.2048
#define MD_MINLENGTH 68.322
#define MD_MAXLENGTH 72.893
#define MD_HALFLENGTH (MD_MINLENGTH+MD_MAXLENGTH)/2.0
#define MD_L0 70.91421
#define MD_ENCODERCOUNTSPERCM 40000.0
#define MD_CALMODEOFFSET 13000
#define MD_CALTUNEMODE 47
#endif


#ifdef QTFTM_FTM2
#define MD_MIRRORROC 83.2048
#define MD_MINLENGTH 67.1
#define MD_MAXLENGTH 72.1
#define MD_HALFLENGTH (MD_MINLENGTH+MD_MAXLENGTH)/2.0
#define MD_L0 69.4141
#define MD_ENCODERCOUNTSPERCM 40000.0
#define MD_CALMODEOFFSET -10000
#define MD_CALTUNEMODE 46
#endif

class MotorDriver : public HardwareObject
{
	Q_OBJECT
public:
	explicit MotorDriver(QObject *parent = nullptr);

    static double calculateModePosition(double f, int mode);
    static double calculateModeFrequency(double position, int mode);
    static QPair<int,int> calcRoughTune(double f, int mode = -1);
    static bool isModeValid(double f, int mode);
	
signals:
    void posUpdate(int);
    void deltaF(double);
    void tuningComplete(bool);
    void canTuneUp(bool);
    void canTuneDown(bool);
    void modeChanged(int);
    void voltageChanged(int);
	
public slots:
    bool canSkipTune(double freq);
    int calcNextMode(double freq, bool above);
    void cavityFreqChanged(double freq);

    int lastTuneVoltage() const;
    int lastTuneAttenuation() const;
    int lastCalVoltage() const;
    void shutUp(bool quiet) { d_quiet = quiet; }
    void measureVoltageNoTune();

protected:
    double d_lastTuneFreq;
    int d_lastTuneAtten;
    int d_lastTuneVoltage;
    int d_lastTuneMode;
    int d_lastTuneWidth;
    int d_lastCalVoltage;
    QDateTime d_lastTuneTime;
    bool d_quiet;

    virtual bool moveToPosition(int pos) =0;
    virtual bool stepMotor(int motorSteps) =0;
    virtual int readAnalog() =0;
    virtual int readPos() =0;

};

#ifdef QTFTM_MOTORDRIVER
#if QTFTM_MOTORDRIVER == 1
#include "antonuccimotordriver.h"
class AntonucciMotorDriver;
typedef AntonucciMotorDriver MotorDriverHardware;
#else
#include "virtualftmwscope.h"
class VirtualMotorDriver;
typedef VirtualMotorDriver MotorDriverHardware;
#endif
#endif

#endif // MOTORDRIVER_H
