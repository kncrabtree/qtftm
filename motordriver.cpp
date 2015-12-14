#include "motordriver.h"
#include <gsl/gsl_math.h>
#include <gsl/gsl_const.h>
#include <QStringList>
#include <QFile>

MotorDriver::MotorDriver(QObject *parent) :
    HardwareObject(parent), d_lastTuneFreq(0.0),
    d_lastTuneAtten(0), d_lastTuneVoltage(0), d_lastTuneMode(0), d_lastTuneWidth(0), d_lastCalVoltage(0), d_quiet(false)
{
    d_key = QString("motorDriver");
}

MotorDriver::~MotorDriver()
{

}

double MotorDriver::calculateModePosition(double f, int mode)
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("motorDriver"));

    double minLength = s.value(QString("minLength"),68.322).toDouble();
    double maxLength = s.value(QString("maxLength"),72.893).toDouble();
    double halfLength = (minLength+maxLength)/2.0;

    s.endGroup();

    //the mode equation can't be solved for position analytically, so must do guess-and-check

    //make sure the chosen mode can be reached with the cavity!
    if(f < calculateModeFrequency(maxLength,mode))
        return -0.5; //mode number is too high
    if(f > calculateModeFrequency(minLength,mode))
        return -1.5; //mode number is too low

    //start by setting limits equal to most extreme possible values, and guess that the mode is in the center
    //after all, the rough tune calculation is trying to choose the mode closest to the center
    double positionBelow = maxLength;
    double positionAbove = minLength;
    double nextPositionToTry = halfLength;

    bool done = false;
    while (!done)
    {
        //calculate frequency for the current guess length
        double fCurrent = calculateModeFrequency(nextPositionToTry,mode);

        if(fabs(fCurrent-f) < .01) //agreement within 0.01 MHz is good enough
            done = true;
        else if (fCurrent > f) //if the frequency is too high, then we can decrease the upper limit
            positionAbove = nextPositionToTry;
        else //likewise if the frequency is too low
            positionBelow = nextPositionToTry;

        if(!done) //if we're not done, then make the new guess to be halfway above the updated limits
            nextPositionToTry = (positionAbove + positionBelow)/2.0;
    }

    return nextPositionToTry;
}

double MotorDriver::calculateModeFrequency(double position, int mode)
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("motorDriver"));
    double mirrorROC = s.value(QString("mirrorROC"),83.2048).toDouble();
    s.endGroup();
    if(position <= 0.0 || mode <= 0)
        return 0.0;
    else
        return (GSL_CONST_CGS_SPEED_OF_LIGHT/1e6/(2.0*position))*((double)mode + acos(1.0-position/mirrorROC)/M_PI);
}

QPair<int, int> MotorDriver::calcRoughTune(double f, int mode)
{

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("motorDriver"));
    double minLength = s.value(QString("minLength"),68.322).toDouble();
    double maxLength = s.value(QString("maxLength"),72.893).toDouble();
    double halfLength = (minLength+maxLength)/2.0;
    double l0 = s.value(QString("l0"),70.91421).toDouble();
    double encoderCountsPerCm = s.value(QString("encoderCountsPerCm"),40000.0).toDouble();
    s.endGroup();


    double position = 0.0; //this will be the cavity length at that mode

    if(mode < 0) //if mode < 0 (default), we need to calculate the mode closest to the center ourselves
    {
        mode = 1; //this will be the mode number
        bool done = false;
        while(!done)
        {
            double test = calculateModeFrequency(minLength,mode);
            if(test > f)
                done = true;
            else
                mode++;
        }

        //variables used in and after loop
        double lastPosition = maxLength; //lastPosition is used during mode selection loop to minimize distance from center.


        while(calculateModeFrequency(maxLength,mode) < f) //loop over range of modes
        {
            //calculate how far this mode is from center
            position = calculateModePosition(f,mode);
            double distanceFromCenter = fabs(position-halfLength);

            //if this one is farther away than the previous one, then choose the previous mode
            if(distanceFromCenter > fabs(lastPosition-halfLength))
            {
                mode--;
                position = lastPosition;
                break;
            }
            else //otherwise, remember how close this one is
                lastPosition = position;

            mode++;
        }
    }
    else //the mode was already specified, so calculate position
        position = calculateModePosition(f,mode);

    //convert position to encoder count
    int encoderPos = (int)round((position-l0)*encoderCountsPerCm);
    return QPair<int,int>(encoderPos,mode);
}

bool MotorDriver::isModeValid(double f, int mode)
{

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("motorDriver"));
    double minLength = s.value(QString("minLength"),68.322).toDouble();
    double maxLength = s.value(QString("maxLength"),72.893).toDouble();
    s.endGroup();

    if(mode < 1)
        return false;

    if(f < 0.01)
        return false;

    double minFreq = calculateModeFrequency(maxLength,mode);
    double maxFreq = calculateModeFrequency(minLength,mode);

    if(f > minFreq && f < maxFreq)
        return true;

    return false;
}

void MotorDriver::initialize()
{
    readCavitySettings();
}

int MotorDriver::calcNextMode(double freq, bool above)
{
    if(fabs(d_lastTuneFreq-freq)>0.01)
        return -1;

    if(above)
    {
        if(isModeValid(freq,d_lastTuneMode+1))
            return d_lastTuneMode+1;
        else
            return -1;
    }
    else
    {
        if(isModeValid(freq,d_lastTuneMode-1))
            return d_lastTuneMode-1;
        else
            return -1;
    }

    return -1;
}

void MotorDriver::cavityFreqChanged(double freq)
{
    bool isSame = fabs(freq-d_lastTuneFreq)<0.01;
    emit canTuneUp(isSame);
    emit canTuneDown(isSame);
}


int MotorDriver::lastTuneVoltage() const
{
    return d_lastTuneVoltage;
}

int MotorDriver::lastTuneAttenuation() const
{
    return d_lastTuneAtten;
}

int MotorDriver::lastCalVoltage() const
{
	return d_lastCalVoltage;
}

int MotorDriver::measureVoltageNoTune()
{
    d_lastTuneVoltage = readAnalog();
    emit voltageChanged(d_lastTuneVoltage);
    return d_lastTuneVoltage;
}

void MotorDriver::readCavitySettings()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    d_mirrorROC = s.value(QString("mirrorROC"),83.2048).toDouble();
    d_minLength = s.value(QString("minLength"),68.322).toDouble();
    d_maxLength = s.value(QString("maxLength"),72.893).toDouble();
    d_halfLength = (d_minLength+d_maxLength)/2.0;
    d_l0 = s.value(QString("l0"),70.91421).toDouble();
    d_encoderCountsPerCm = s.value(QString("encoderCountsPerCm"),40000.0).toDouble();
    d_calOffset = s.value(QString("calOffset"),13000).toInt();
    d_calMode = s.value(QString("calMode"),47).toInt();

    if(!s.contains(QString("mirrorROC")))
        s.setValue(QString("mirrorROC"),d_mirrorROC);
    if(!s.contains(QString("minLength")))
        s.setValue(QString("minLength"),d_minLength);
    if(!s.contains(QString("maxLength")))
        s.setValue(QString("maxLength"),d_maxLength);
    if(!s.contains(QString("l0")))
        s.setValue(QString("l0"),d_l0);
    if(!s.contains(QString("encoderCountsPerCm")))
        s.setValue(QString("encoderCountsPerCm"),d_encoderCountsPerCm);
    if(!s.contains(QString("calOffset")))
        s.setValue(QString("calOffset"),d_calOffset);
    if(!s.contains(QString("calMode")))
        s.setValue(QString("calMode"),d_calMode);

    s.endGroup();
}
