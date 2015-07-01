#include "pulsegenerator.h"
#include <math.h>

PulseGenerator::PulseGenerator(QObject *parent) :
    Rs232Instrument(QString("pulseGenerator"),QString("Pulse Generator"),parent)
{

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.setValue(QString(d_key).append("/minWidth"),0.004);
    s.setValue(QString(d_key).append("/maxWidth"),1.0e5);
    s.setValue(QString(d_key).append("/minDelay"),0.0);
    s.setValue(QString(d_key).append("/maxDelay"),1.0e5);
    s.sync();
}


void PulseGenerator::initialize()
{
    if(pulseConfig.isEmpty())
    {
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(1,QString("Gas"),true,0.0,350.0,ActiveHigh));
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(2,QString("DC"),true,50.0,900.0,ActiveHigh));
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(3,QString("MW"),true,1300.0,1.0,ActiveLow));
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(4,QString("DR"),false,1301.0,200.0,ActiveHigh));
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(5,QString("Aux1"),false,0.0,50.0,ActiveHigh));
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(6,QString("Aux2"),false,0.0,50.0,ActiveLow));
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(7,QString("Aux3"),false,0.0,50.0,ActiveHigh));
        pulseConfig.append(PulseGenerator::PulseChannelConfiguration(8,QString("Aux4"),false,0.0,50.0,ActiveHigh));
    }

    Rs232Instrument::initialize();
    if(!testConnection())
        emit newSettings(pulseConfig);
}

bool PulseGenerator::testConnection()
{
    if(!Rs232Instrument::testConnection())
    {
        emit connected(false,QString("RS232 error."));
        return false;
    }

    setReadOptions(100,true,QByteArray("\r\n"));
    QByteArray resp = queryCmd(QString("*IDN?\n"));

    if(resp.isEmpty())
    {
        emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
        return false;
    }
    if(!resp.startsWith("9518+"))
    {
        emit connected(false,QString("%1 response invalid. Received: %2").arg(d_prettyName).arg(QString(resp)));
        return false;
    }
    emit logMessage(QString("%1 ID response: %2").arg(d_prettyName).arg(QString(resp)));



    applySettings();


    //enforce certain settings
    //gas and mw pulses must be on
    setChannelSetting(1,Enabled,true);
    setChannelSetting(3,Enabled,true);

    //mw pulse must be active low
    setChannelSetting(3,Active,ActiveLow);

    emit connected();
    return true;
}

void PulseGenerator::sleep(bool b)
{
    if(b)
        pGenWriteCmd(QString(":SPULSE:STATE 0\n"));
    else
        pGenWriteCmd(QString(":SPULSE:STATE 1\n"));

    HardwareObject::sleep(b);
}

QVariant PulseGenerator::setChannelSetting(const int ch, const PulseGenerator::Setting s, const QVariant val)
{
    //only write and read command if it's different than the current setting!
    //communication is SLOW!
	if(ch-1>=pulseConfig.length())
		return QVariant();
	switch(s)
	{
	case Delay:
        if(fabs(val.toDouble()-pulseConfig.at(ch-1).delay)>0.001)
        {
            pGenWriteCmd(QString(":PULSE%1:DELAY %2\n").arg(ch).arg(val.toDouble()/1e6,0,'f',9));
            readChannelSetting(ch,Delay);
        }
        return pulseConfig[ch-1].delay;
		break;
	case Width:
        if(fabs(val.toDouble()-pulseConfig.at(ch-1).width)>0.001)
        {
            pGenWriteCmd(QString(":PULSE%1:WIDTH %2\n").arg(ch).arg(val.toDouble()/1e6,0,'f',9));
            readChannelSetting(ch,Width);
        }
        return pulseConfig[ch-1].width;
        break;
	case Enabled:
        if(val.toBool() != pulseConfig.at(ch-1).enabled)
        {
            //gas pulse and mw pulse cannot be disabled
            if(val.toBool() || ch-1==0 || ch-1==2)
                pGenWriteCmd(QString(":PULSE%1:STATE 1\n").arg(ch));
            else
                pGenWriteCmd(QString(":PULSE%1:STATE 0\n").arg(ch));
            readChannelSetting(ch,Enabled);
        }
        return pulseConfig[ch-1].enabled;
		break;
	case Active:
        if((ActiveLevel)val.toInt() != pulseConfig.at(ch-1).active)
        {
            //mw pulse cannot be inverted
            if((ActiveLevel)val.toInt() == ActiveHigh || ch-1==2)
                pGenWriteCmd(QString(":PULSE%1:POLARITY NORM\n").arg(ch));
            else
                pGenWriteCmd(QString(":PULSE%1:POLARITY INV\n").arg(ch));

            readChannelSetting(ch,Active);
        }
        return pulseConfig[ch-1].active;
		break;
	case Name:
		pulseConfig[ch-1].channelName = val.toString();
		return readChannelSetting(ch,Name);
        break;

	}

	return QVariant();

}

PulseGenerator::PulseChannelConfiguration PulseGenerator::setChannelAll(const int ch, const double delay, const double width, const bool enabled, const ActiveLevel a)
{
	if(ch-1 < pulseConfig.size())
	{
        setChannelSetting(ch,Delay,QVariant(delay));
        setChannelSetting(ch,Width,QVariant(width));
        setChannelSetting(ch,Active,QVariant((int)a));
        setChannelSetting(ch,Enabled,QVariant(enabled));

        return pulseConfig[ch-1];
	}

	return PulseGenerator::PulseChannelConfiguration();
}

PulseGenerator::PulseChannelConfiguration PulseGenerator::setChannelAll(const PulseGenerator::PulseChannelConfiguration p)
{
	if(p.channel-1 < pulseConfig.size())
        return setChannelAll(p.channel,p.delay,p.width,p.enabled,p.active);

	return PulseGenerator::PulseChannelConfiguration();
}

QList<PulseGenerator::PulseChannelConfiguration> PulseGenerator::setAll(const QList<PulseGenerator::PulseChannelConfiguration> l)
{
	if(l.size() <= pulseConfig.size())
	{
		for(int i=0; i<l.size(); i++)
			setChannelAll(l.at(i));

        emit newSettings(pulseConfig);
        return pulseConfig;
	}

	return QList<PulseGenerator::PulseChannelConfiguration>();
}

QVariant PulseGenerator::readChannelSetting(const int ch, const PulseGenerator::Setting s)
{
    bool blocked = signalsBlocked();
    blockSignals(false);
	QVariant out;
    QByteArray resp;
	if(ch-1 < pulseConfig.size())
	{
		switch(s)
		{
		case Delay:
            resp = queryCmd(QString(":PULSE%1:DELAY?\n").arg(ch));
//            emit logMessage(QString(":PULSE%1:DELAY? response: %2").arg(ch).arg(QString(resp.trimmed())));
            if(resp.isEmpty())
            {
                emit hardwareFailure();
                out = QVariant();
            }
            else
            {
                out = QVariant(resp.trimmed().toDouble()*1e6);
                pulseConfig[ch-1].delay = out.toDouble();
            }
			break;
		case Width:
            resp = queryCmd(QString(":PULSE%1:WIDTH?\n").arg(ch));
//            emit logMessage(QString(":PULSE%1:WIDTH? response: %2").arg(ch).arg(QString(resp.trimmed())));
            if(resp.isEmpty())
            {
                emit hardwareFailure();
                out = QVariant();
            }
            else
            {
                out = QVariant(resp.trimmed().toDouble()*1e6);
                pulseConfig[ch-1].width = out.toDouble();
            }
            break;
		case Enabled:
            resp = queryCmd(QString(":PULSE%1:STATE?\n").arg(ch));
//            emit logMessage(QString(":PULSE%1:STATE? response: %2").arg(ch).arg(QString(resp.trimmed())));
            if(resp.isEmpty())
            {
                emit hardwareFailure();
                out = QVariant();
            }
            else
            {
                out = (bool)resp.trimmed().toInt();
                pulseConfig[ch-1].enabled = out.toBool();
            }
            break;
		case Active:
            resp = queryCmd(QString(":PULSE%1:POLARITY?\n").arg(ch));
//            emit logMessage(QString(":PULSE%1:POLARITY? response: %2").arg(ch).arg(QString(resp.trimmed())));
            if(resp.isEmpty())
            {
                emit hardwareFailure();
                out = QVariant();
            }
            else
            {
                if(QString(resp).startsWith(QString("NORM"),Qt::CaseInsensitive))
                    out = QVariant((int)PulseGenerator::ActiveHigh);
                else
                    out = QVariant((int)PulseGenerator::ActiveLow);
                pulseConfig[ch-1].active = (ActiveLevel)out.toInt();
            }
            break;
		case Name:
			out = QVariant(pulseConfig.at(ch-1).channelName);
			break;
		}
	}

    blockSignals(blocked);
	emit newChannelSetting(ch,s,out);
	return out;
}

PulseGenerator::PulseChannelConfiguration PulseGenerator::readChannelAll(const int ch)
{
	if(ch-1 < pulseConfig.size())
	{
		bool blocked = signalsBlocked();
		blockSignals(true);
		readChannelSetting(ch,Delay);
		readChannelSetting(ch,Width);
		readChannelSetting(ch,Enabled);
		readChannelSetting(ch,Active);
        readChannelSetting(ch,Name);

		if(!blocked)
		{
			blockSignals(false);
			emit newChannelSettingAll(pulseConfig.at(ch-1));
		}
		return pulseConfig.at(ch-1);
	}

	return PulseGenerator::PulseChannelConfiguration();
}

QList<PulseGenerator::PulseChannelConfiguration> PulseGenerator::readAll()
{
    blockSignals(true);
	for(int i=1; i<9; i++)
		readChannelAll(i);
    blockSignals(false);

	emit newSettings(pulseConfig);

    return pulseConfig;
}

QList<PulseGenerator::PulseChannelConfiguration> PulseGenerator::configureForTuning()
{
    //copy current settings to return
    QList<PulseGenerator::PulseChannelConfiguration> out(pulseConfig);

    //disable everything but the gas pulse
    //note, MW channel won't actually be disabled, but that's ok.
    for(int i=1;i<pulseConfig.size();i++)
        setChannelSetting(i+1,Enabled,false);

    return out;
}

void PulseGenerator::applySettings()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    pulseConfig[4].channelName = s.value(QString("chEName"),QString("Aux1")).toString();
    pulseConfig[5].channelName = s.value(QString("chFName"),QString("Aux2")).toString();
    pulseConfig[6].channelName = s.value(QString("chGName"),QString("Aux3")).toString();
    pulseConfig[7].channelName = s.value(QString("chHName"),QString("Aux4")).toString();

    blockSignals(true);

    if(s.value(QString("chEActive"),1).toInt() == 0)
        setChannelSetting(5,Active,ActiveLow);
    else
        setChannelSetting(5,Active,ActiveHigh);

    if(s.value(QString("chFActive"),1).toInt() == 0)
        setChannelSetting(6,Active,ActiveLow);
    else
        setChannelSetting(6,Active,ActiveHigh);

    if(s.value(QString("chGActive"),1).toInt() == 0)
        setChannelSetting(7,Active,ActiveLow);
    else
        setChannelSetting(7,Active,ActiveHigh);

    if(s.value(QString("chHActive"),1).toInt() == 0)
        setChannelSetting(8,Active,ActiveLow);
    else
        setChannelSetting(8,Active,ActiveHigh);

    blockSignals(false);

    setRepRate(s.value(QString("repRate"),6.0).toDouble());

    s.endGroup();
    readAll();
}

void PulseGenerator::setRepRate(double rr)
{
    if(rr <= 0.001 || rr > 15.0)
        return;

    if(!pGenWriteCmd(QString(":SPULSE:PERIOD %1\n").arg(1.0/rr,0,'f',8)))
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not set %1 repetition rate to %2 Hz (Period = %3 s).").arg(d_prettyName).arg(rr,0,'f',1).arg(1.0/rr,0,'f',8));
    }
}

bool PulseGenerator::pGenWriteCmd(QString cmd)
{
    int max_attempts = 10;
    for(int i=0;i<max_attempts;i++)
    {
        QByteArray resp = queryCmd(cmd);
        if(resp.isEmpty())
            return false;

        if(resp.startsWith("ok"))
            return true;
    }
    return false;
}

