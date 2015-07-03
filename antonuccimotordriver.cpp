#include "antonuccimotordriver.h"

AntonucciMotorDriver::AntonucciMotorDriver(QObject *parent) :
    MotorDriver(parent)
{
	d_lastTuneFreq = 0.0;
	d_lastTuneAtten = 0;
	d_lastTuneVoltage = 0;
	d_lastTuneMode = 0;
	d_lastTuneWidth = 0;
	d_lastCalVoltage = 0;
	d_quiet = false;
    d_key = QString("motorDriver");
}

void AntonucciMotorDriver::initialize()
{
    //note that read options will need to change for fine tuning, since that takes over 1 second to finish
    p_comm->setReadOptions(1500,true,QByteArray("\n\r"));
    p_comm->initialize();
    testConnection();
}

bool AntonucciMotorDriver::testConnection()
{
    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    if(p_comm->device()->bytesAvailable())
	   p_comm->device()->readAll();

    QByteArray resp = p_comm->queryCmd(QString("v"));
    if(!resp.startsWith("v:"))
    {
        emit connected(false,QString("Invalid ID response. (Response: %1)").arg(QString(resp)));
        return false;
    }

    readPos();

    emit connected();
    return true;
}

void AntonucciMotorDriver::tune(double freq, int currentAttn, int mode)
{
    //gather some initial information
    QPair<int,int> modePair = calcRoughTune(freq,mode);

    emit canTuneUp(isModeValid(freq,modePair.second+1));
    emit canTuneDown(isModeValid(freq,modePair.second-1));
    if(modePair.second != d_lastTuneMode)
        emit modeChanged(modePair.second);

    readPos();
    bool doRoughTune = true, doFineTune = true;
    int tuningVMax;


    //decide whether rough tuning and/or fine tuning are necessary
    if(fabs(freq-d_lastTuneFreq)<0.5 && d_lastTuneMode == modePair.second)
    {
        //if we're already really close, we might not even need to move
        doRoughTune = false;
        if(fabs(freq-d_lastTuneFreq)<0.1)
        {
            int an = readAnalog();
            if(an<0)
            {
                emit tuningComplete(false);
                return;
            }

            int attenDiff = currentAttn-d_lastTuneAtten;
            if(attenDiff != 0)
                doRoughTune = true;
            else if(an>=9*d_lastTuneVoltage/10)
            {
                doFineTune = false;
                d_lastTuneVoltage = qMax(an,d_lastTuneVoltage);
                d_lastTuneAtten = currentAttn;
                emit voltageChanged(d_lastTuneVoltage);
            }
            else if(an>30)
                tuningVMax = (int)round((double)d_lastTuneVoltage/pow(10.0,attenDiff/10)); //convert last voltage in terms of current attenuation;
            else
                doRoughTune = true; //we've lost the mode; need to rough tune
        }
        else //try moving mirror the amount we think is needed, then fine tune
        {
            //estimate where motor needs to go
            double lastLength = calculateModePosition(d_lastTuneFreq,d_lastTuneMode);
            double targetLength = calculateModePosition(freq,modePair.second);
            double diff = targetLength - lastLength; //distance to move, in cm

            //~2 motor steps per encoder tick
            int stepsToMove = (int)round(diff*MD_ENCODERCOUNTSPERCM*2.0);
            if( abs(stepsToMove) < 2000 ) //don't do something stupid!
            {
                if(!stepMotor(stepsToMove))
                {
                    emit tuningComplete(false);
                    return;
                }
            }

            int an = readAnalog();
            if(an<0)
            {
                emit tuningComplete(false);
                return;
            }

            if(an>30)
                tuningVMax = d_lastTuneVoltage;
            else
                doRoughTune = true; //lost mode, rough tune
        }
    }

    int peakWidth = 100, peakPos = modePair.first, peakIndex = -1;
    if(doRoughTune)
    {
        d_lastTuneVoltage = -1; // make last tuning votlage invalid!
        doFineTune = true; //make double damn certain that a fine tune follows a rough tune
        QStringList itResult = roughTune(modePair.first, freq);

        if(itResult.size()<9)
        {
            if(!d_quiet)
                emit logMessage(QString("Rough tune failed at frequency %1. Check calibration.").arg(freq,0,'f',3),QtFTM::LogError);
            emit tuningComplete(false);
            return;
        }

        bool ok = false;
        int tv = itResult.at(4).split("=").at(1).trimmed().toInt(&ok);
        if(ok)
            tuningVMax = tv*330/1024;
        else
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not parse max voltage in intermediate tune response."),QtFTM::LogError);
            emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(4)).arg(QString(itResult.at(4).toLatin1().toHex())),QtFTM::LogError);
            emit tuningComplete(false);
            return;
        }

        ok = false;
        int p = itResult.at(5).split("=").at(1).trimmed().toInt(&ok);
        if(ok)
            peakWidth = p*2; //convert to motor steps
        else
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not parse peak width in intermediate tune response"),QtFTM::LogError);
            emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(5)).arg(QString(itResult.at(5).toLatin1().toHex())),QtFTM::LogError);
            emit tuningComplete(false);
            return;
        }

        int ind = itResult.at(8).split("=").at(1).trimmed().toInt(&ok);
        if(ok)
            peakIndex = ind;
        else
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not parse peak index in intermediate tune respons."),QtFTM::LogError);
            emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(8)).arg(QString(itResult.at(8).toLatin1().toHex())),QtFTM::LogError);
            emit tuningComplete(false);
            return;
        }

        int loc = itResult.at(6).split("=").at(1).trimmed().toInt(&ok);
        if(ok)
            peakPos = loc;
        else
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not parse peak location in intermediate tune response."),QtFTM::LogError);
            emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(6)).arg(QString(itResult.at(6).toLatin1().toHex())),QtFTM::LogError);
            emit tuningComplete(false);
            return;
        }

        //only in ta mode
        //we've left off at the end of the sweep, so try to move back to peak position (40 motor steps per index)
//        stepMotor(-(2000-peakIndex*40+peakWidth));
    }
    else
        peakWidth = d_lastTuneWidth;


    if(doFineTune)
    {
        int attempts = 0, maxAttempts = 3;
        bool success = false;
        if(freq > 40000) maxAttempts = 2;
        while(attempts<maxAttempts && !success)
        {
            attempts++;
            if(attempts > 1)
            {
                if(doRoughTune)
                    moveToPosition(peakPos);
                else
                {
                    //kick motor before retrying
                    int sign = (qrand() % 2) == 0 ? -1 : 1;
                    int mag = 100 + (qrand()%100);
                    stepMotor(sign*mag);
                }
            }

            success = fineTune(tuningVMax,peakWidth,freq,modePair.second,currentAttn);
        }

        if(!success)
        {
            //don't abort scan if fine tuning failed... just give a warning.
            if(!d_quiet)
                emit logMessage(QString("Fine tuning unsuccessful! Frequency: %1 MHz, Mode %2, Position = %3.").arg(freq,0,'f',3).arg(modePair.second).arg(readPos()),QtFTM::LogWarning);
        }
    }

//    int finalPos = readPos();
//    QFile f(QString("/home/data/QtFTM/tuningLog.txt"));
//    if(f.open(QIODevice::Append))
//    {
//        f.write(QString("%1\t%2\t%3\t%4\t%5\t%6\n").arg(freq,0,'f',3).arg(startingPos).arg(posGuess).arg(finalPos).arg(finalPos-startingPos).arg(finalPos-posGuess).toLatin1());
//        f.close();
//    }

    emit tuningComplete(true);
}

QStringList AntonucciMotorDriver::roughTune(int posGuess, bool calibrating, double freq)
{
    QStringList out;
//    int guessToPutOut = posGuess/4;     // PRAA March 24 2015 2:15 pm
//    emit logMessage(QString("Heading towards position: %1").arg(guessToPutOut));
    ///TODO: Use slope and offset terms to refine position guess?
    if(!moveToPosition(posGuess-500))
        return out;

    QStringList itResult;

    int attempts = 0, maxAttempts = 10, searchIndex = 0;
    if (freq >=40000) maxAttempts = 2;
    do
    {
        attempts++;
        QByteArray resp = intermediateTune();

        if(resp.isEmpty() || !resp.startsWith("t:"))
        {
            emit hardwareFailure();
            emit logMessage(QString("Communication error while performing intermediate tune."),QtFTM::LogError);
            emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
            return out;
        }

        if(resp.trimmed().endsWith('?'))
        {
           if(calibrating)
               return out;

           searchIndex++;

          if(searchIndex == maxAttempts)
          {
              if(!d_quiet)
                  emit logMessage(QString("Rough tuning failed after %1 attempts! Calibration is probably needed.").arg(maxAttempts),QtFTM::LogWarning);
              ///TODO: Automatically calibrate?
              return out;
          }

          int direction = searchIndex%2?-1:1;
          int offset = 1000*direction*((searchIndex+1)/2);

          if(!moveToPosition(posGuess-500+offset))
              return out;

          continue;

        }

        QStringList l = QString(resp).split(QString(","));
        if(l.size() < 9)
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not parse intermediate tune response."),QtFTM::LogError);
            emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
            return out;
        }

        //look at "too_close" flag, and redo intermediate tune if needed
        if(l.at(l.size()-1).trimmed().endsWith("0"))
        {
            itResult = l;
            break;
        }

        else if(l.at(l.size()-1).trimmed().endsWith("-1"))
        {
//            emit logMessage(QString("Guess too high."),QtFTM::LogNormal);
            //peak was too close to lower edge. Move lower and try again
            resp.clear();
            int newPos = l.at(0).split("=").at(1).trimmed().toInt()-1333;
            if(!moveToPosition(newPos))
                return out;
        }
        else
        {
//            emit logMessage(QString("Guess too low."),QtFTM::LogNormal);
            //peak was too close to upper edge. Move higher and try again
            resp.clear();
            int newPos = l.at(0).split("=").at(1).trimmed().toInt()-667;
            if(!moveToPosition(newPos))
                return out;
        }
    } while(attempts < maxAttempts && searchIndex < maxAttempts);

    if(attempts == maxAttempts)
    {
       if(!calibrating && !d_quiet)
            emit logMessage(QString("Could not settle on peak in %1 attempts. Calibration is probably needed.").arg(attempts),QtFTM::LogWarning);
        ///TODO: Automatically calibrate?
        return out;
    }

    if(searchIndex > 0 && !d_quiet)
        emit logMessage(QString("Rough tuning required a blind search. Calibration is recommended if this message persists."),QtFTM::LogWarning);

    return itResult;

}

QByteArray AntonucciMotorDriver::intermediateTune()
{
    p_comm->writeCmd(QString("td50\n"));

    bool done = false;
    QByteArray resp;
    while(!done)
    {
        if(!p_comm->device()->waitForReadyRead(10000))
            break;
        else
        {
            resp.append(p_comm->device()->readAll());
            if(resp.endsWith("\n\r"))
                done = true;
        }
    }

    if(!done && resp.startsWith("t:") && testConnection())
    {
        //retry...
        //NOTE: possibility of infinite recursion here!
        return intermediateTune();
    }

    if(resp.isEmpty() || !resp.startsWith("t:") || !done)
    {
        emit hardwareFailure();
        emit logMessage(QString("Communication error while performing intermediate tune."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return QByteArray();
    }

    return resp;
}

bool AntonucciMotorDriver::fineTune(int targetVoltage, int width, double freq, int currentMode, int currentAttn)
{
    int startingStepSize = width/4;
    int minStepSize = 10;
    int maxSteps = 30, loopMax = 0, finalVoltage = 0;
    bool tuningDone = false;
    int steps = 0, stepSize = startingStepSize, direction = 1;

    if(freq >=40000)
        maxSteps = 6;  // Test to see if this helps tuning at high frequencies Mar 20 2015 PRAA MCM

    //convert FWHM to difference in frequency....
    int pos = readPos();
    if(pos < -10000000)
        return false;

    //compute resonant frequencies for modes detuned by HWHM each (the current position is used to get the total length close), use a fraction of the difference for detuning
    //note width is FWHM in motor steps, need to divide by 4 for HWHM in encoder ticks
    double lAbove = ((double)pos+(double)width/4.0)/MD_ENCODERCOUNTSPERCM+MD_L0;
    double lBelow = ((double)pos-(double)width/4.0)/MD_ENCODERCOUNTSPERCM+MD_L0;
    double fAbove = calculateModeFrequency(lAbove,currentMode);
    double fBelow = calculateModeFrequency(lBelow,currentMode);

    double startingDf = fabs(fAbove-fBelow)/4.0, df=startingDf;

    while(steps < maxSteps && !tuningDone)
    {
        //look at voltage to lower frequency
        emit deltaF(-df);
        int vBelow = readAnalog();

        //look at voltage to higher frequency
        emit deltaF(df);
        int vAbove = readAnalog();

        //now read voltage at current position
        emit deltaF(0.0);
        int vCenter = readAnalog();

        //keep track of the highest voltage seen so far
        loopMax = qMax( qMax( qMax(vCenter,vAbove), vBelow), loopMax);

        //we're done if the center voltage is greater than either offset voltage, and it's at least 90% of the target, and 90% of the highest value we've seen
        // above comment is from Kyle.  MArch 20 2015, Mike and Paul find that it doesn't work above 40000 or so, so take the line above. PRAA
        if(((freq >41000) && (vCenter >= ((86*targetVoltage)/100)) ) // && vCenter >= vBelow && vCenter >= vAbove && vCenter >= 9*loopMax/10
        || (vCenter >= 9*targetVoltage/10 && vCenter >= vBelow && vCenter >= vAbove && vCenter >= 9*loopMax/10))
        {
            finalVoltage = vCenter;
            tuningDone = true;
        }
        else //we need to step the mirror
        {
            if(vAbove > vBelow) //this means we need to move in the positive direction
            {
                if(steps>0 && direction == -1) //this means we're reversing direction (probably converging)
                {
                    if(loopMax >= 9*targetVoltage/10) //yep, we're converging
                    {
                        if(stepSize == minStepSize && vCenter >= 9*loopMax/10) // this is good enough
                        {
                            finalVoltage = vCenter;
                            tuningDone = true;
                            break;
                        }
                    }

                    if(stepSize == minStepSize) // we've somehow converged on a local max, take bigger steps
                    {
                        startingStepSize*=2;
                        stepSize = startingStepSize;
                        startingDf*=2.0;
                        df = startingDf;
                    }
                    else
                    {
                        stepSize = qMax(stepSize/2,minStepSize); // we're trying to settle on peak
                        df/=2.0;
                    }
                }
                //set the direction for the move
                direction = 1;
            }
            else
            {
                if(steps>0 && direction == 1) //this means we're reversing direction (probably converging)
                {
                    if(loopMax >= 9*targetVoltage/10) //yep, we're converging
                    {
                        if(stepSize == minStepSize && vCenter >= 9*loopMax/10) // this is good enough
                        {
                            finalVoltage = vCenter;
                            tuningDone = true;
                            break;
                        }
                    }

                    if(stepSize == minStepSize) // we've somehow converged on a local max, take bigger steps
                    {
                        startingStepSize*=2;
                        stepSize = startingStepSize;
                        startingDf*=2.0;
                        df = startingDf;
                    }
                    else
                    {
                        stepSize = qMax(stepSize/2,minStepSize); // we're trying to settle on peak
                        df/=2.0;
                    }
                }
                //set the direction for the move
                direction = -1;
            }

            //NOTE: should check for error here
            stepMotor(direction*stepSize);
            readPos();
        }
        steps++;
    }

    if(currentAttn > 0 && finalVoltage < 300)
        tuningDone = false;

    d_lastTuneVoltage = finalVoltage;
    if(tuningDone)
    {
//        emit logMessage(QString("Frequency: %1 MHz, Steps: %2, Pos: %3").arg(freq,0,'f',3).arg(steps).arg(readPos()));
        d_lastTuneFreq = freq;
        d_lastTuneTime = QDateTime::currentDateTime();
        d_lastTuneWidth = width;
        d_lastTuneMode = currentMode;
        d_lastTuneAtten = currentAttn;
        emit voltageChanged(d_lastTuneVoltage);
        return true;
    }
    else
        emit voltageChanged(d_lastTuneVoltage);

    return false;
}

void AntonucciMotorDriver::calibrate()
{
    QByteArray resp;
    //the mode we're looking for is usually around -10000 or so, but some searching might be needed
    bool done = false;
    int i = 0;
    QStringList itResult;

    //first step in calibration is to find the index mark... this can take a while
    if(p_comm->device()->bytesAvailable())
        p_comm->device()->readAll();

    p_comm->writeCmd(QString("I"));
    bool initDone = false;
    while(!initDone)
    {
        if(!p_comm->device()->waitForReadyRead(30000)) //shouldn't take more than 30 seconds though
        {
            emit logMessage(QString("Could not find home switch for %1 calibration. This is a serious error, and likely means a switch is broken or the microcontroller is failing.").arg(d_prettyName),QtFTM::LogError);
            emit tuningComplete(false);
            return;
        }
        resp.append(p_comm->device()->readAll());
        if(resp.endsWith("\n\r"))
            initDone = true;
    }

    while(!done && i<40)
    {
        // want to look at -10000, -11000, -9000, -12000, -8000 etc...
        int direction = i%2?-1:1;
        int tuneOffset = 1000*direction*((i+1)/2);
        i++;
       itResult = roughTune(MD_CALMODEOFFSET + tuneOffset,true, 10030);
        if(itResult.size() > 9)
            done = true;
    }

    if(!done)
    {
        if(!d_quiet)
            emit logMessage(QString("Could not find correct mode for tuning. Verify that synthesizer is on, tuned to 10000.000 MHz, power set to +10 dBm, and that the attenuation is appropriately set."),QtFTM::LogError);
        emit tuningComplete(false);
        return;
    }

    bool ok = false;
    int tuningVMax = 0, peakWidth = 0, peakPos = 0, peakIndex = -1;
    int tv = itResult.at(4).split("=").at(1).trimmed().toInt(&ok);
    if(ok)
        tuningVMax = tv*330/1024;
    else
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse max voltage in intermediate tune response."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(4)).arg(QString(itResult.at(4).toLatin1().toHex())),QtFTM::LogError);
        emit tuningComplete(false);
        return;
    }

    ok = false;
    int p = itResult.at(5).split("=").at(1).trimmed().toInt(&ok);
    if(ok)
        peakWidth = p*2; //convert to motor steps
    else
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse peak width in intermediate tune response."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(5)).arg(QString(itResult.at(5).toLatin1().toHex())),QtFTM::LogError);
        emit tuningComplete(false);
        return;
    }

    int ind = itResult.at(8).split("=").at(1).trimmed().toInt(&ok);
    if(ok)
        peakIndex = ind;
    else
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse peak index in intermediate tune response."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(8)).arg(QString(itResult.at(8).toLatin1().toHex())),QtFTM::LogError);
        emit tuningComplete(false);
        return;
    }

    int loc = itResult.at(6).split("=").at(1).trimmed().toInt(&ok);
    if(ok)
        peakPos = loc;
    else
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse peak location in intermediate tune response."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(itResult.at(6)).arg(QString(itResult.at(6).toLatin1().toHex())),QtFTM::LogError);
        emit tuningComplete(false);
        return;
    }

    //only in ta mode
    //we've left off at the end of the sweep, so try to move back to peak position (40 motor steps per index)
//        stepMotor(-(2000-peakIndex*40+peakWidth));

    int attempts = 0, maxAttempts = 3;
    bool success = false;
    while(attempts<maxAttempts && !success)
    {
        attempts++;
       success = fineTune(tuningVMax,peakWidth,10030.0,MD_CALTUNEMODE,0);

        if(!success)
        {
            //try kicking it back to the right place
            moveToPosition(peakPos);
//            rt.second*=3/2;
        }
    }

    d_lastCalVoltage = d_lastTuneVoltage;
    if(success)
    {
        d_lastTuneAtten = 0;
        d_lastTuneMode = 0;
        d_lastTuneFreq = 0.0;
        d_lastTuneVoltage = 0;
        d_lastTuneWidth = 0;
    }
    else
    {
        //don't abort scan if fine tuning failed... just give a warning.
        if(!d_quiet)
            emit logMessage(QString("Fine tuning during calibration was unsuccessful! Try again, or peak cavity by hand and issue R command to %1 from terminal.").arg(d_prettyName),QtFTM::LogWarning);
    }

    //reset encoder to 0 at this position
    p_comm->queryCmd(QString("R"));
    readPos();

//    QFile f(QString("/home/data/QtFTM/tuningLog.txt"));
//    if(f.open(QIODevice::Append))
//    {
//        f.write(QString("\ncalibrated\n\n").toLatin1());
//        f.close();
//    }

    emit tuningComplete(true);
}

bool AntonucciMotorDriver::moveToPosition(int pos)
{
    int currentPos = readPos();
    if(pos < -10000000)
        return false;

    int direction = (pos > currentPos ? 1 : -1);

    p_comm->writeCmd(QString("X%1\n").arg(pos));

    bool done = false;
    int count = 0;
    QByteArray resp;
    while(!done && count < 30000/10)
    {
        if(!p_comm->device()->waitForReadyRead(10))
        {
            count++;
            currentPos += direction*115; //this is a rough approximation from guess-and-check
            emit posUpdate(currentPos);
        }
        else
        {
            resp.append(p_comm->device()->readAll());
            if(resp.endsWith("\n\r"))
                done = true;
        }
    }

    if(!done)
    {
        emit hardwareFailure();
        emit logMessage(QString("Mirror move did not complete in 30 seconds.").arg(d_prettyName),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return false;
    }

    readPos();

    return true;
}

bool AntonucciMotorDriver::stepMotor(int motorSteps)
{
    QByteArray resp = p_comm->queryCmd(QString("M%1\n").arg(motorSteps));
    if(!resp.startsWith("M:"))
    {
        emit hardwareFailure();
        emit logMessage(QString("Communication error while stepping motor."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return false;
    }

    return true;
}

int AntonucciMotorDriver::readAnalog()
{
    int analog = getAnalogReading();
    if(analog < 0)
        return -1;
    bool done = false;
    while(!done)
    {
	   p_comm->device()->waitForReadyRead(10);
        int newAnalog = getAnalogReading();
        if(newAnalog < 0)
            return -1;
        if(abs(newAnalog-analog) < 10)
            done = true;

        analog = newAnalog;
    }

    return analog;
}

int AntonucciMotorDriver::readPos()
{
    QByteArray resp = p_comm->queryCmd(QString("P"));
    if(resp.isEmpty() || !resp.startsWith("P:"))
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read position."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return -100000000;
    }

    bool ok = false;
    QStringList l = QString(resp.trimmed()).split("=");
    int out = l.at(l.size()-1).trimmed().toInt(&ok);
    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse position response."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return -100000000;
    }

    emit posUpdate(out);
    return out;
}

int AntonucciMotorDriver::getAnalogReading()
{
    QByteArray resp = p_comm->queryCmd(QString("A"));
    if(resp.isEmpty() || !resp.startsWith("A:"))
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read analog voltage."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return -1;
    }

    bool ok = false;
    QStringList l = QString(resp.trimmed()).split(",");
    int out = l.at(l.size()-1).trimmed().toInt(&ok);
    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse analog voltage."),QtFTM::LogError);
        emit logMessage(QString("Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())),QtFTM::LogError);
        return -1;
    }

    // converting sum of ten 10-bit digitizer readings to mV (3300 mV is full scale reading)
    return out*330/1024;
}
