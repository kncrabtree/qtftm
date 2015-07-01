#ifndef MOTORDRIVER_H
#define MOTORDRIVER_H

#include "rs232instrument.h"
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

class MotorDriver : public Rs232Instrument
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
    void initialize();
	bool testConnection();
    bool canSkipTune(double freq);
    int calcNextMode(double freq, bool above);
    void cavityFreqChanged(double freq);

    void tune(double freq, int currentAttn, int mode = -1);
    QStringList roughTune(int posGuess, bool calibrating = false, double freq = -1); // let roughTune know freq so it can change if F in top region Mar 20 PRAA
    QByteArray intermediateTune();
    bool fineTune(int targetVoltage, int width, double freq, int currentMode, int currentAttn);

    void calibrate();
    int lastTuneVoltage() const;
    int lastTuneAttenuation() const;
    int lastCalVoltage() const;
    void shutUp(bool quiet) { d_quiet = quiet; }
    void measureVoltageNoTune();

private:
    double d_lastTuneFreq;
    int d_lastTuneAtten;
    int d_lastTuneVoltage;
    int d_lastTuneMode;
    int d_lastTuneWidth;
    int d_lastCalVoltage;
    QDateTime d_lastTuneTime;
    bool d_quiet;

    bool moveToPosition(int pos);
    bool stepMotor(int motorSteps);
    int readAnalog();
    int readPos();
    int waitForSlope();

};

#endif // MOTORDRIVER_H
