#include "scan.h"
#include <QSharedData>
#include <QSettings>
#include <QApplication>
#include <math.h>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

/*!
 \brief Data storage for Scan

 Self-explanatory data storage.

*/
class ScanData : public QSharedData {
public:
	ScanData() : number(-1), ts(QDateTime::currentDateTime()), ftFreq(-1.0), ftAtten(-1), drFreq(-1.0), drPower(-100.0),
	   targetShots(0), completedShots(0), fid(Fid()), initialized(false), saved(false), aborted(false), dummy(false), skipTune(false),
      postTuneShots(0), postTuneDelayShots(0), tuningVoltage(-1), cavityVoltage(-1), protectionDelayTime (-1),
	  scopeDelayTime(-1), dipoleMoment(0.0), magnet(false), dcVoltage(0) {}
	ScanData(const ScanData &other) :
		QSharedData(other), number(other.number), ts(other.ts), ftFreq(other.ftFreq), ftAtten(other.ftAtten), drFreq(other.drFreq),
	   drPower(other.drPower), flowConfig(other.flowConfig),
		pulseConfig(other.pulseConfig), targetShots(other.targetShots), completedShots(other.completedShots),
        fid(other.fid), initialized(other.initialized), saved(other.saved), aborted(other.aborted), dummy(other.dummy),
       skipTune(other.skipTune), postTuneShots(other.postTuneShots), postTuneDelayShots(other.postTuneDelayShots),
       tuningVoltage(other.tuningVoltage), cavityVoltage(other.cavityVoltage),
        protectionDelayTime(other.protectionDelayTime), scopeDelayTime(other.scopeDelayTime), dipoleMoment(other.dipoleMoment),
	   magnet(other.magnet), dcVoltage(other.dcVoltage) {}
	~ScanData() {}

	int number;
	QDateTime ts;

	double ftFreq;
	int ftAtten;
	double drFreq;
	double drPower;

    FlowConfig flowConfig;
    PulseGenConfig pulseConfig;

	int targetShots;
	int completedShots;

	Fid fid;
	bool initialized;
	bool saved;
	bool aborted;
	bool dummy;
	bool skipTune;
    int postTuneShots;
    int postTuneDelayShots;

    int tuningVoltage;
    int cavityVoltage;

    int protectionDelayTime;
    int scopeDelayTime;

    double dipoleMoment;
    bool magnet;

    int dcVoltage;

};

Scan::Scan() : data(new ScanData)
{
}

Scan::Scan(int num) : data(new ScanData)
{
	parseFile(num);
}

Scan::Scan(const Scan &rhs) : data(rhs.data)
{
}

Scan &Scan::operator=(const Scan &rhs)
{
	if (this != &rhs)
		data.operator=(rhs.data);
    return *this;
}

Scan Scan::settingsFromPrevious(int num)
{
    Scan out;
    Scan other(num);

    out.setAttenuation(other.attenuation());
    out.setDcVoltage(other.dcVoltage());
    out.setDipoleMoment(other.dipoleMoment());
    out.setDrFreq(other.drFreq());
    out.setDrPower(other.drPower());
    out.setMagnet(other.magnet());
    out.setFtFreq(other.ftFreq());
    out.setProtectionDelayTime(other.protectionDelayTime());
    out.setScopeDelayTime(other.scopeDelayTime());
    out.setPulseConfiguration(other.pulseConfiguration());
    out.setTargetShots(other.targetShots());

    return out;
}

bool Scan::operator ==(const Scan &other) const
{
	return data == other.data;
}

Scan::~Scan()
{
}

int Scan::number() const
{
	return data->number;
}

QDateTime Scan::timeStamp() const
{
	return data->ts;
}

Fid Scan::fid() const
{
	return data->fid;
}

double Scan::ftFreq() const
{
	return data->ftFreq;
}

double Scan::drFreq() const
{
	return data->drFreq;
}

int Scan::attenuation() const
{
	return data->ftAtten;
}

double Scan::dipoleMoment() const
{
	return data->dipoleMoment;
}

bool Scan::magnet() const
{
	return data->magnet;
}

int Scan::dcVoltage() const
{
	return data->dcVoltage;
}

double Scan::drPower() const
{
	return data->drPower;
}

double Scan::pressure() const
{
    return data->flowConfig.pressure();
}

QStringList Scan::gasNames() const
{
    QStringList out;
    for(int i=0; i<data->flowConfig.size(); i++)
        out.append(data->flowConfig.setting(i,QtFTM::FlowSettingName).toString());

    return out;
}

QList<double> Scan::gasFlows() const
{
	QList<double> out;
	for(int i=0; i<data->flowConfig.size(); i++)
		out.append(data->flowConfig.setting(i,QtFTM::FlowSettingFlow).toDouble());
    return out;
}

FlowConfig Scan::flowConfig() const
{
    return data->flowConfig;
}

double Scan::repRate() const
{
    return data->pulseConfig.repRate();
}

PulseGenConfig Scan::pulseConfiguration() const
{
	return data->pulseConfig;
}

int Scan::completedShots() const
{
	return data->completedShots;
}

int Scan::targetShots() const
{
	return data->targetShots;
}

bool Scan::isInitialized() const
{
	return data->initialized;
}

bool Scan::isAcquisitionComplete() const
{
	if(!isInitialized())
		return false;
	else
		return (data->completedShots >= data->targetShots) || data->aborted;
}

bool Scan::isSaved() const
{
	return data->saved;
}

bool Scan::isAborted() const
{
	return data->aborted;
}

bool Scan::isDummy() const
{
	return data->dummy;
}

bool Scan::skipTune() const
{
    return data->skipTune;
}

bool Scan::tuneDelay() const
{
    if(data->skipTune)
        return false;

    return data->postTuneShots < data->postTuneDelayShots;
}

int Scan::postTuneDelayShots() const
{
    return data->postTuneDelayShots;
}

int Scan::tuningVoltage() const
{
    return data->tuningVoltage;
}

int Scan::cavityVoltage() const
{
    return data->cavityVoltage;
}

int Scan::protectionDelayTime() const
{
    return data->protectionDelayTime;
}

int Scan::scopeDelayTime() const
{
    return data->scopeDelayTime;
}

void Scan::setNumber(int n)
{
	data->number = n;
}

void Scan::increment()
{
    data->postTuneShots++;
    if(!tuneDelay())
        data->completedShots++;
}

void Scan::setFid(const Fid f)
{
	data->fid = f;
}

void Scan::setProbeFreq(const double f)
{
	data->fid.setProbeFreq(f);
}

void Scan::setFtFreq(const double f)
{
	data->ftFreq = f;
}

void Scan::setDrFreq(const double f)
{
	data->drFreq = f;
}

void Scan::setAttenuation(const int a)
{
	data->ftAtten = a;
}

void Scan::setCavityVoltage(const int v)
{
   data->cavityVoltage = v;
}

void Scan::setProtectionDelayTime(const int v)
{
   data->protectionDelayTime = v;
}

void Scan::setScopeDelayTime (const int v)
{
   data->scopeDelayTime = v;
}

void Scan::setDipoleMoment( const double v)
{
    data->dipoleMoment = v;
}

void Scan::setFlowConfig(const FlowConfig c)
{
    data->flowConfig = c;
}

void Scan::setDrPower(const double p)
{
	data->drPower = p;
}

void Scan::setPressure(const double p)
{
	data->flowConfig.setPressure(p);
}

void Scan::setTargetShots(int n)
{
    data->targetShots = n;
}

void Scan::setPulseConfiguration(const PulseGenConfig p)
{
	data->pulseConfig = p;
}

void Scan::initializationComplete()
{
	data->ts = QDateTime::currentDateTime();
	data->initialized = true;
}

void Scan::save()
{

	//figure out scan number and where to save data
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	data->number = s.value(QString("scanNum"),0).toInt()+1;

	int dirMillionsNum = (int)floor((double) number()/1000000.0);
	int dirThousandsNum = (int)floor((double) number()/1000.0);

	QString savePath = s.value(QString("savePath"),QString(".")).toString();
	QDir d(savePath + QString("/scans/%1/%2").arg(dirMillionsNum).arg(dirThousandsNum));
	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
		{
			//this is bad... abort!
			data->aborted=true;
			return;
		}
	}

	//create output file
	QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(number()));

	if(!f.open(QIODevice::WriteOnly))
	{
		//this is also bad... abort!
		data->aborted = true;
		return;
	}

	QTextStream t(&f);

	//write header and column heading
	t << scanHeader();
	t << QString("\nfid%1").arg(number());
	t.setRealNumberNotation(QTextStream::ScientificNotation);

	//this controls how many digits are printed after decimal.
	//in principle, an 8 bit digitizer requires only 3 digits (range = -127 to 128)
	//For each ~factor of 10 averages, we need ~one more digit of precision
	//This starts at 7 digits (sci notation; 6 places after decimal), and adds 1 for every factor of 10 shots.
	int logFactor = 0;
	if(completedShots() > 0)
		logFactor = (int)floor(log10((double)completedShots()));
	t.setRealNumberPrecision(6+logFactor);

	//write data
    for(int i=0; i<fid().size(); i++)
		t << QString("\n") << fid().at(i);

	t.flush();
	f.close();

	//increment scan number
	s.setValue(QString("scanNum"),data->number);
    s.sync();
	data->saved = true;
}

void Scan::abortScan()
{
	data->aborted = true;
}

void Scan::setDummy()
{
	data->dummy = true;
}

void Scan::setSkiptune(bool b)
{
    data->skipTune = b;
}

void Scan::setPostTuneDelayShots(int s)
{
    data->postTuneDelayShots = s;
}

void Scan::setTuningVoltage(int v)
{
	data->tuningVoltage = v;
}

void Scan::setMagnet(bool b)
{
	data->magnet = b;
}

void Scan::setDcVoltage(int v)
{
	data->dcVoltage = v;
}

QString Scan::scanHeader() const
{
	QString out;
	QTextStream t(&out);

    t.setRealNumberPrecision(12);

	t << QString("#Scan\t") << number() << QString("\tFTM%1\n").arg(QTFTM_SPECTROMETER);
	t << QString("#Date\t") << timeStamp().toString() << QString("\t\n");
	t << QString("#Shots\t") << completedShots() << QString("\t\n");
	t << QString("#Cavity freq\t") << ftFreq() << QString("\tMHz\n");
	t << QString("#Skipped Tuning\t") << skipTune() << QString("\t\n");
    t << QString("#Post Tuning Delay") << postTuneDelayShots() << QString("\tshots\n");
    t << QString("#Tuning Voltage\t") << tuningVoltage() << QString("\tmV\n");
	t << QString("#Attenuation\t") << attenuation() << QString("\tdB\n");
    t << QString("#Dipole Moment\t") << dipoleMoment() << QString("\tD\n");
    t << QString("#Cavity Voltage\t") << cavityVoltage() << QString("\tmV\n");
    t << QString("#Protection Delay\t") << protectionDelayTime() << QString("\tus\n");
    t << QString("#Scope Delay\t") << scopeDelayTime() << QString("\tus\n");
    t << QString("#Magnet enabled\t") << magnet() << QString("\t\n");
//	t << QString("#DR enabled\t") << pulseConfiguration().at(3).enabled << QString("\t\n");
	t << QString("#DR freq\t") << drFreq() << QString("\tMHz\n");
	t << QString("#DR power\t") << drPower() << QString("\tdBm\n");
	t << QString("#DC voltage") << dcVoltage() << QString("\tV\t\n");
	t << QString("#Probe freq\t") << fid().probeFreq() << QString("\tMHz\n");
	t << QString("#FID spacing\t") << fid().spacing() << QString("\ts\n");
	t << QString("#FID points\t") << fid().size() << QString("\t\n");
    t << QString("#Rep rate\t") << repRate() << QString("\tHz\n");
    t << data->flowConfig.headerString();
    t << data->pulseConfig.headerString();

	t.flush();
	return out;
}

void Scan::parseFile(int num)
{
	if(num<1)
		return;

	int dirMillionsNum = (int)floor((double) num/1000000.0);
	int dirThousandsNum = (int)floor((double) num/1000.0);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QString savePath = s.value(QString("savePath"),QString(".")).toString();
	QFile f(savePath + QString("/scans/%1/%2/%3.txt").arg(dirMillionsNum).arg(dirThousandsNum).arg(num));

	if(!f.exists())
		return;

	if(!f.open(QIODevice::ReadOnly))
		return;

	QVector<double> fidData;

	while(!f.atEnd())
	{
		QString line(f.readLine());
		if(line.startsWith(QString("#FID points")))
			fidData.reserve(line.split(QChar(0x09)).at(1).toInt());
		else if(line.startsWith(QString("#")))
			parseFileLine(line);
		else if(line.startsWith(QString("fid")))
			break;
	}

	while(!f.atEnd())
		fidData.append(QString(f.readLine()).toDouble());

	data->fid.setData(fidData);
	data->saved = true;
	data->initialized = true;
	data->targetShots = completedShots();

}

void Scan::parseFileLine(QString s)
{
	QStringList sl = s.split(QChar(0x09));
	if(sl.size()<2)
		return;

	QString key = sl.at(0);
	QString val = sl.at(1);

    if((key.startsWith(QString("#Scan"))) && (key.endsWith(QString("can"))))
		data->number = val.toInt();
	else if(key.startsWith(QString("#Date")))
		data->ts = QDateTime::fromString(val);
	else if(key.startsWith(QString("#Shots")))
		data->completedShots = val.toInt();
	else if(key.startsWith(QString("#Cavity freq")))
		data->ftFreq = val.toDouble();
	else if(key.startsWith(QString("#Skipped")))
		data->skipTune = (bool)val.toInt();
    else if(key.startsWith(QString("#Post")))
        data->postTuneDelayShots = val.toInt();
    else if(key.startsWith(QString("#Tuning")))
        data->tuningVoltage = val.toInt();
    else if(key.startsWith(QString("#Cavity Voltage"), Qt::CaseInsensitive))
        data->cavityVoltage = val.toInt();
    else if(key.startsWith(QString("#Protection Delay"), Qt::CaseInsensitive))
        data->protectionDelayTime = val.toInt();
    else if(key.startsWith(QString("#Scope Delay"), Qt::CaseInsensitive))
        data->scopeDelayTime = val.toInt();
    else if(key.startsWith(QString("#Attenuation")))
        data->ftAtten = val.toInt();
    else if(key.startsWith(QString("#Dipole Moment")))
        data->dipoleMoment = val.toDouble();
	else if(key.startsWith(QString("#Magnet")))
		data->magnet = (bool)val.toInt();
	else if(key.startsWith(QString("#DR freq")))
		data->drFreq = val.toDouble();
	else if(key.startsWith(QString("#DR power")))
		data->drPower = val.toDouble();
    else if(key.startsWith(QString("DC voltage")))
	    data->dcVoltage = val.toDouble();
	else if(key.startsWith(QString("#Pressure")))
		data->flowConfig.setPressure(val.toDouble());
	else if(key.startsWith(QString("#Probe freq")))
		data->fid.setProbeFreq(val.toDouble());
	else if(key.startsWith(QString("#FID spacing")))
		data->fid.setSpacing(val.toDouble());
    else if(key.startsWith(QString("#Rep rate")))
        data->pulseConfig.setRepRate(val.toDouble());
	else if(key.startsWith(QString("#Gas")))
	{
        int ch = key.split(QChar(0x20)).at(1).toInt()-1;
		if(key.endsWith(QString("name")))
		{
			if(ch >= data->flowConfig.size())
			{
				while(ch >= data->flowConfig.size())
					data->flowConfig.add(0.0,QString(""));
			}

			data->flowConfig.set(ch,QtFTM::FlowSettingName,val);
		}
		else if(key.endsWith(QString("flow")))
		{
			if(ch >= data->flowConfig.size())
			{
				while(ch >= data->flowConfig.size())
					data->flowConfig.add(0.0,QString(""));
			}

			data->flowConfig.set(ch,QtFTM::FlowSettingFlow,val.toDouble());
		}
    }
	else if(key.startsWith(QString("#Pulse ch")))
	{
        int ch = key.split(QChar(0x20)).at(2).toInt()-1;
		if(key.endsWith(QString("name")))
		{
            if(ch >= data->pulseConfig.size())
		  {
			  while(ch >= data->pulseConfig.size())
				  data->pulseConfig.add(QString(""),false,0.0,1.0,QtFTM::PulseLevelActiveHigh);
		  }
		  data->pulseConfig.set(ch,QtFTM::PulseName,val);
		}
		else if(key.endsWith(QString("level")))
		{
			if(ch >= data->pulseConfig.size())
			{
				while(ch >= data->pulseConfig.size())
					data->pulseConfig.add(QString(""),false,0.0,1.0,QtFTM::PulseLevelActiveHigh);
			}

		  if(val.contains(QString("High"),Qt::CaseInsensitive))
			  data->pulseConfig.set(ch,QtFTM::PulseLevel,QtFTM::PulseLevelActiveHigh);
		  else
			  data->pulseConfig.set(ch,QtFTM::PulseLevel,QtFTM::PulseLevelActiveLow);
		}
		else if(key.endsWith(QString("enabled")))
		{
			if(ch >= data->pulseConfig.size())
			{
				while(ch >= data->pulseConfig.size())
					data->pulseConfig.add(QString(""),false,0.0,1.0,QtFTM::PulseLevelActiveHigh);
			}

		  data->pulseConfig.set(ch,QtFTM::PulseEnabled,static_cast<bool>(val.toInt()));
		}
		else if(key.endsWith(QString("delay")))
		{
			if(ch >= data->pulseConfig.size())
			{
				while(ch >= data->pulseConfig.size())
					data->pulseConfig.add(QString(""),false,0.0,1.0,QtFTM::PulseLevelActiveHigh);
			}

		  data->pulseConfig.set(ch,QtFTM::PulseDelay,val.toDouble());
		}
		else if(key.endsWith(QString("width")))
		{
			if(ch >= data->pulseConfig.size())
			{
				while(ch >= data->pulseConfig.size())
					data->pulseConfig.add(QString(""),false,0.0,1.0,QtFTM::PulseLevelActiveHigh);
			}

		  data->pulseConfig.set(ch,QtFTM::PulseWidth,val.toDouble());
		}
	}
}
