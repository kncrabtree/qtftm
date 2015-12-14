#ifndef MOTORDRIVER_H
#define MOTORDRIVER_H

#include "hardwareobject.h"
#include <QPair>
#include <QDateTime>
#include <QStringList>

class MotorDriver : public HardwareObject
{
	Q_OBJECT
public:
	explicit MotorDriver(QObject *parent = nullptr);
    virtual ~MotorDriver();

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
    virtual void initialize();
    int calcNextMode(double freq, bool above);
    void cavityFreqChanged(double freq);
    virtual void tune(double freq, int currentAttn, int mode) =0;
    virtual void calibrate() =0;

    int lastTuneVoltage() const;
    int lastTuneAttenuation() const;
    int lastCalVoltage() const;
    void shutUp(bool quiet) { d_quiet = quiet; }
    int measureVoltageNoTune();
    void readCavitySettings();

protected:
    double d_lastTuneFreq;
    int d_lastTuneAtten;
    int d_lastTuneVoltage;
    int d_lastTuneMode;
    int d_lastTuneWidth;
    int d_lastCalVoltage;
    QDateTime d_lastTuneTime;
    bool d_quiet;

    double d_mirrorROC;
    double d_minLength;
    double d_maxLength;
    double d_halfLength;
    double d_l0;
    double d_encoderCountsPerCm;
    int d_calOffset;
    int d_calMode;

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
#include "virtualmotordriver.h"
class VirtualMotorDriver;
typedef VirtualMotorDriver MotorDriverHardware;
#endif
#endif

#endif // MOTORDRIVER_H
