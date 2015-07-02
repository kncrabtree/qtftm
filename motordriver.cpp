#include "motordriver.h"
#include <gsl/gsl_math.h>
#include <gsl/gsl_const.h>
#include <QStringList>
#include <QFile>

MotorDriver::MotorDriver(QObject *parent) :
    HardwareObject(parent), d_lastTuneFreq(0.0),
    d_lastTuneAtten(0), d_lastTuneVoltage(0), d_lastTuneMode(0), d_lastTuneWidth(0), d_lastCalVoltage(0), d_quiet(false)
{
}

double MotorDriver::calculateModePosition(double f, int mode)
{
    //the mode equation can't be solved for position analytically, so must do guess-and-check

    //make sure the chosen mode can be reached with the cavity!
    if(f < calculateModeFrequency(MD_MAXLENGTH,mode))
        return -0.5; //mode number is too high
    if(f > calculateModeFrequency(MD_MINLENGTH,mode))
        return -1.5; //mode number is too low

    //start by setting limits equal to most extreme possible values, and guess that the mode is in the center
    //after all, the rough tune calculation is trying to choose the mode closest to the center
    double positionBelow = MD_MAXLENGTH;
    double positionAbove = MD_MINLENGTH;
    double nextPositionToTry = MD_HALFLENGTH;

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
    if(position <= 0.0 || mode <= 0)
        return 0.0;
    else
        return (GSL_CONST_CGS_SPEED_OF_LIGHT/1e6/(2.0*position))*((double)mode + acos(1.0-position/MD_MIRRORROC)/M_PI);
}

QPair<int, int> MotorDriver::calcRoughTune(double f, int mode)
{
    double position; //this will be the cavity length at that mode

    if(mode < 0) //if mode < 0 (default), we need to calculate the mode closest to the center ourselves
    {
        mode = 1; //this will be the mode number
        bool done = false;
        while(!done)
        {
            double test = calculateModeFrequency(MD_MINLENGTH,mode);
            if(test > f)
                done = true;
            else
                mode++;
        }

        //variables used in and after loop
        double lastPosition = MD_MAXLENGTH; //lastPosition is used during mode selection loop to minimize distance from center.


        while(calculateModeFrequency(MD_MAXLENGTH,mode) < f) //loop over range of modes
        {
            //calculate how far this mode is from center
            position = calculateModePosition(f,mode);
            double distanceFromCenter = fabs(position-MD_HALFLENGTH);

            //if this one is farther away than the previous one, then choose the previous mode
            if(distanceFromCenter > fabs(lastPosition-MD_HALFLENGTH))
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
    int encoderPos = (int)round((position-MD_L0)*MD_ENCODERCOUNTSPERCM);
    return QPair<int,int>(encoderPos,mode);
}

bool MotorDriver::isModeValid(double f, int mode)
{
    if(mode < 1)
        return false;

    if(f < 0.01)
        return false;

    double minFreq = calculateModeFrequency(MD_MAXLENGTH,mode);
    double maxFreq = calculateModeFrequency(MD_MINLENGTH,mode);

    if(f > minFreq && f < maxFreq)
        return true;

    return false;
}


bool MotorDriver::canSkipTune(double freq)
{
    if(fabs(freq-d_lastTuneFreq) < 0.01 && d_lastTuneTime.addSecs(1800) > QDateTime::currentDateTime() && d_lastTuneVoltage>300 && d_lastTuneVoltage<3000)
        return true;// Note! d_lastTuneTime.addSecs(300) was replaced by 1800  May 15 PRAA

    return false;
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

void MotorDriver::measureVoltageNoTune()
{
    d_lastTuneVoltage = readAnalog();
    emit voltageChanged(d_lastTuneVoltage);
	if(d_lastTuneVoltage < 0)
		emit tuningComplete(false);
	else
        emit tuningComplete(true);
}
