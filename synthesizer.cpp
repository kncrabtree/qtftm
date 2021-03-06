#include "synthesizer.h"

Synthesizer::Synthesizer(QObject *parent) :
    HardwareObject(parent), d_mult(1.0), d_offset(0.0), d_hardwareMinFreq(50), d_hardwareMaxFreq(26500)
{
}

Synthesizer::~Synthesizer()
{

}


void Synthesizer::initialize()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

    int band = s.value(QString("%1/%2/currentBand").arg(d_key).arg(d_subKey),0).toInt();
    d_mult = s.value(QString("%1/%2/bands/%3/mult").arg(d_key).arg(d_subKey).arg(band+1),1.0).toDouble();
    d_offset = s.value(QString("%1/%2/bands/%3/offset").arg(d_key).arg(d_subKey).arg(band+1),0.0).toDouble();
}

void Synthesizer::updateBandFromUi()
{
    bool ok = false;
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

    int band = s.value(QString("%1/%2/currentBand").arg(d_key).arg(d_subKey),0).toInt(&ok);
    if(!ok)
    {
        emit logMessage(QString("Could not read current band from settings. Please check settings file."),QtFTM::LogError);
        return;
    }

    double m = s.value(QString("%1/%2/bands/%3/mult").arg(d_key).arg(d_subKey).arg(band+1),1.0).toDouble(&ok);
    if(!ok)
    {
        emit logMessage(QString("Could not read current band multiplier from settings. Please check settings file."),QtFTM::LogError);
        return;
    }


    double o = s.value(QString("%1/%2/bands/%3/offset").arg(d_key).arg(d_subKey).arg(band+1),1.0).toDouble(&ok);
    if(!ok)
    {
        emit logMessage(QString("Could not read current band offset from settings. Please check settings file."),QtFTM::LogError);
        return;
    }

    switchBand(band);

    d_mult = m;
    d_offset = o;

    readFreq();
}


double Synthesizer::realToRaw(double f)
{
    double outputFreq = f/d_mult + d_offset;
    if(outputFreq < d_hardwareMinFreq || outputFreq > d_hardwareMaxFreq)
    {
        emit logMessage(QString("Calculated raw frequency (%1 MHz) is outside range of synthesizer (%2 - %3 MHz). Please check band settings and try again.")
                        .arg(QString::number(outputFreq,'f',3)).arg(QString::number(d_hardwareMinFreq,'f',3)).arg(QString::number(d_hardwareMaxFreq,'f',3)),QtFTM::LogError);
        return -1.0;
    }

    return outputFreq;
}

double Synthesizer::rawToReal(double f)
{
    return (f-d_offset)*d_mult;
}

double Synthesizer::readFreq()
{
    double f = rawToReal(readSynthFreq());
    emit frequencyUpdate(f);
    return f;
}

double Synthesizer::setFreq(double d)
{
    if(!calculateBand(d))
        return -1.0;

    return setFreqForce(d);
}

double Synthesizer::readPower()
{
    double p = readSynthPower();
    emit powerUpdate(p);
    return p;
}

double Synthesizer::setPower(double p)
{
    if(p < d_hardwareMinPower || p > d_hardwareMaxPower)
    {
        emit logMessage(QString("Requested power (%1) is outside hardware range (%2 - %3).")
                        .arg(p).arg(d_hardwareMinPower).arg(d_hardwareMaxPower),QtFTM::LogWarning);
        return -1e200;
    }

    return setSynthPower(p);
}

bool Synthesizer::calculateBand(double f)
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

    //convert from real frequency to raw using current band settings
    bool ok = false;
    int band = s.value(QString("%1/%2/currentBand").arg(d_key).arg(d_subKey),0).toInt(&ok);
    if(!ok || band < 0)
    {
        emit logMessage(QString("Could not read current band. Please check settings file."),QtFTM::LogError);
        return false;
    }

    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    s.beginReadArray(QString("bands"));
    s.setArrayIndex(band);

    ok = false;
    double bandMin = s.value(QString("min")).toDouble(&ok);
    if(!ok || bandMin < 50.0)
    {
        emit logMessage(QString("Could not read current band minimum frequency. Please check settings file."),QtFTM::LogError);
        return false;
    }

    ok = false;
    double bandMax = s.value(QString("max")).toDouble(&ok);
    if(!ok || bandMax < 50.0)
    {
        emit logMessage(QString("Could not read current band maximum frequency. Please check settings file."),QtFTM::LogError);
        return false;
    }

    ok = false;
    double mult = s.value(QString("mult"),1.0).toDouble(&ok);
    if(!ok || mult < 1.0)
    {
        emit logMessage(QString("Could not read current band multiplier. Please check settings file."),QtFTM::LogError);
        return false;
    }

    ok = false;
    double offset = s.value(QString("offset"),0.0).toDouble(&ok);
    if(!ok)
    {
        emit logMessage(QString("Could not read current band offset. Please check settings file."),QtFTM::LogError);
        return false;
    }
    s.endArray();

    if(f < bandMin || f > bandMax || s.value(QString("autoSwitch"),false).toBool())
    {
        //can we switch?
        if(s.value(QString("autoSwitch"),false).toBool())
        {
            //is there another band that would be valid?
            bool success = false;
            int numBands = s.beginReadArray(QString("bands")), newBand = -1;
            for(int i=0;i<numBands;i++)
            {
                s.setArrayIndex(i);
                double thisBandMin = s.value(QString("min")).toDouble(&ok);
                if(!ok)
                    continue;

                double thisBandMax = s.value(QString("max")).toDouble(&ok);
                if(!ok)
                    continue;

                if(f<thisBandMin || f>thisBandMax)
                    continue;

                //check to make sure that the next band (if applicable) doesn't have priority
                if(i<numBands-1)
                {
                    s.setArrayIndex(i+1);
                    if(s.value(QString("switch"),0).toInt() == 1)
                    {
                        double nextBandMin = s.value(QString("min")).toDouble(&ok);
                        if(ok && f>nextBandMin)
                        {
                            success = true;
                            newBand = i+1;
                            break;
                        }
                    }
                }
                newBand = i;
                success = true;
                break;
            }

            if(success)
            {
                switchBand(newBand);
                s.setArrayIndex(newBand);
                ok = false;
                bandMin = s.value(QString("min")).toDouble(&ok);
                if(!ok || bandMin < 50.0)
                {
                    emit logMessage(QString("Could not read current band minimum frequency. Please check settings file."),QtFTM::LogError);
                    return false;
                }

                ok = false;
                bandMax = s.value(QString("max")).toDouble(&ok);
                if(!ok || bandMax < 50.0)
                {
                    emit logMessage(QString("Could not read current band maximum frequency. Please check settings file."),QtFTM::LogError);
                    return false;
                }

                ok = false;
                mult = s.value(QString("mult"),1.0).toDouble(&ok);
                if(!ok || mult < 1.0)
                {
                    emit logMessage(QString("Could not read current band multiplier. Please check settings file."),QtFTM::LogError);
                    return false;
                }

                ok = false;
                offset = s.value(QString("offset"),0.0).toDouble(&ok);
                if(!ok)
                {
                    emit logMessage(QString("Could not read current band offset. Please check settings file."),QtFTM::LogError);
                    return false;
                }
                s.endArray();
            }
            else
            {
                emit logMessage(QString("Could not find a valid band at frequency %1. Check band settings and try again.").arg(QString::number(f,'f',3)),QtFTM::LogError);
                return false;
            }
        }
        else //frequency is outside range, and automatic switching is disabled.
        {
            emit logMessage(QString("Invalid frequency. Frequency = %1 MHz, current min = %2 MHz, current max = %3 MHz.Automatic band switching is disabled. Please change band and try again.")
                            .arg(QString::number(f,'f',3)).arg(QString::number(bandMin,'f',3)).arg(QString::number(bandMax,'f',3)),QtFTM::LogError);
            return false;
        }
    }

    s.endGroup();
    s.endGroup();

    d_mult = mult;
    d_offset = offset;
    return true;
}

void Synthesizer::switchBand(int band)
{
    emit triggerBandChange(band);
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    s.setValue(QString("currentBand"),band);
    if(!s.value(QString("autoSwitch"),false).toBool())
    {
        s.setValue(QString("min"),s.value(QString("bands/%1/min").arg(band+1)).toDouble());
        s.setValue(QString("max"),s.value(QString("bands/%1/max").arg(band+1)).toDouble());
        emit rangeChanged();
    }
    s.endGroup();
    s.endGroup();
    s.sync();
}

double Synthesizer::setFreqForce(double f)
{
    double rawFreq = realToRaw(f);

    if(rawFreq<d_hardwareMinFreq || rawFreq>d_hardwareMaxFreq)
    {
        emit logMessage(QString("Requested raw frequency (%1) is outside hardware range (%2 - %3).")
                        .arg(rawFreq).arg(d_hardwareMinFreq).arg(d_hardwareMaxFreq),QtFTM::LogWarning);
        return -1.0;
    }

    return setSynthFreq(rawFreq);
}

