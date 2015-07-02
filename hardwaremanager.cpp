#include "hardwaremanager.h"
#include <QTimer>
#include <QApplication>

HardwareManager::HardwareManager(QObject *parent) :
    QObject(parent), d_waitingForScanTune(false), d_waitingForCalibration(false), d_tuningOldA(-1), d_numGpib(-1), d_firstInitialization(true)
{
}

HardwareManager::~HardwareManager()
{
    //note, the hardwareObjects are deleted when the threads exit
    while(!d_hardwareList.isEmpty())
    {
        QPair<HardwareObject*,QThread*> p = d_hardwareList.takeFirst();
        if(p.second != nullptr)
        {
            p.second->quit();
            p.second->wait();
        }
        else
            p.first->deleteLater();
    }
}


void HardwareManager::initializeHardware()
{
	emit statusMessage(QString("Initializing..."));

    scope = new OscilloscopeHardware();
	connect(scope,&Oscilloscope::fidAcquired,this,&HardwareManager::scopeWaveAcquired);
    connect(scope,&Oscilloscope::statusMessage,this,&HardwareManager::statusMessage);
    d_hardwareList.append(qMakePair(scope,new QThread(this)));

    gpib = new GpibControllerHardware();
//	connect(glc,&GpibLanController::ftmSynthFreq,this,&HardwareManager::ftmSynthUpdate);
//	connect(glc,&GpibLanController::probeFreqUpdate,this,&HardwareManager::probeFreqUpdate);
//    connect(glc,&GpibLanController::rangeChanged,this,&HardwareManager::synthRangeChanged);
//    connect(this,&HardwareManager::ftmSynthChangeBandFromUi,glc,&GpibLanController::ftmSynthChangeBandFromUi);
//#ifndef CONFIG_NODRSYNTH
//	connect(glc,&GpibLanController::drSynthFreq,this,&HardwareManager::drSynthFreqUpdate);
//	connect(glc,&GpibLanController::drSynthPower,this,&HardwareManager::drSynthPwrUpdate);
//    connect(this,&HardwareManager::drSynthChangeBandFromUi,glc,&GpibLanController::drSynthChangeBandFromUi);
//#endif
//	connect(glc,&GpibLanController::testComplete,this,&HardwareManager::testComplete);
//    connect(glc,&GpibLanController::numInstruments,this,&HardwareManager::setNumGpibInstruments);
    d_hardwareList.append(qMakePair(gpib,new QThread(this)));

    attn = new AttenuatorHardware();
	connect(attn,&Attenuator::attnUpdate,this,&HardwareManager::attenUpdate);
    connect(attn,&Attenuator::attenFileParseSuccess,this,&HardwareManager::attnLoadSuccess);
    connect(attn,&Attenuator::taattnUpdate,this,&HardwareManager::taattenUpdate);
    d_hardwareList.append(qMakePair(attn,nullptr));

    pin = new PDGHardware();
    connect(pin,&PinSwitchDriveDelayGenerator::protectionDelayUpdate,this,&HardwareManager::hmProtectionDelayUpdate);
    connect(pin,&PinSwitchDriveDelayGenerator::scopeTriggerDelayUpdate,this,&HardwareManager::hmScopeDelayUpdate);
    connect(this,&HardwareManager::setProtectionDelayFromUI,pin,&PinSwitchDriveDelayGenerator::setProtectionDelay);
    connect(this,&HardwareManager::setScopeDelayFromUI,pin,&PinSwitchDriveDelayGenerator::setScopeDelay);
    d_hardwareList.append(qMakePair(pin,new QThread(this)));

    md = new MotorDriverHardware();
    connect(md,&MotorDriver::posUpdate,this,&HardwareManager::mirrorPosUpdate);
//    connect(md,&MotorDriver::deltaF,gpib,&GpibLanController::ftmDeltaFreq,Qt::BlockingQueuedConnection);
    connect(md,&MotorDriver::tuningComplete,this,&HardwareManager::cavityTuneComplete);
//    connect(gpib,&GpibLanController::ftmSynthFreq,md,&MotorDriver::cavityFreqChanged);
    connect(md,&MotorDriver::canTuneUp,this,&HardwareManager::canTuneUp);
    connect(md,&MotorDriver::canTuneDown,this,&HardwareManager::canTuneDown);
    connect(md,&MotorDriver::modeChanged,this,&HardwareManager::modeChanged);
    connect(md,&MotorDriver::voltageChanged,this,&HardwareManager::tuningVoltageChanged);
    d_hardwareList.append(qMakePair(md,new QThread(this)));

    iob = new IOBoard();
    connect(iob,&IOBoard::triggered,scope,&Oscilloscope::sendCurveQuery);
//    connect(gpib,&GpibLanController::ftmSynthBandChanged,iob,&IOBoard::ftmSynthBand,Qt::BlockingQueuedConnection);
    connect(iob,&IOBoard::magnetUpdate,this,&HardwareManager::magnetUpdate);
    connect(this,&HardwareManager::setMagnetFromUI,iob,&IOBoard::setMagnet);
    d_hardwareList.append(qMakePair(iob,new QThread(this)));


    fc = new FlowControllerHardware();
    connect(fc,&FlowController::flowUpdate,this,&HardwareManager::flowUpdate);
    connect(fc,&FlowController::pressureUpdate,this,&HardwareManager::pressureUpdate);
    connect(fc,&FlowController::flowSetpointUpdate,this,&HardwareManager::flowSetpointUpdate);
    connect(fc,&FlowController::pressureSetpointUpdate,this,&HardwareManager::pressureSetpointUpdate);
    connect(fc,&FlowController::pressureControlMode,this,&HardwareManager::pressureControlMode);
    connect(this,&HardwareManager::setPressureControlMode,fc,&FlowController::setPressureControlMode);
    connect(this,&HardwareManager::setGasName,fc,&FlowController::setChannelName);
    connect(this,&HardwareManager::setFlowSetpoint,&FlowController::setFlowSetpoint);
    connect(this,&HardwareManager::setPressureSetpoint,&FlowController::setPressureSetpoint);
    d_hardwareList.append(qMakePair(fc,new QThread(this)));

    pGen = new PulseGenerator();
    connect(pGen,&PulseGenerator::newChannelSetting,this,&HardwareManager::pGenChannelSetting);
    connect(pGen,&PulseGenerator::newChannelSettingAll,this,&HardwareManager::pGenChannelAll);
    connect(pGen,&PulseGenerator::newSettings,this,&HardwareManager::pGenAll);
    d_hardwareList.append(qMakePair(pGen,new QThread(this)));

	//write arrays of the connected devices for use in the Hardware Settings menu
	//first array is for all objects accessible to the hardware manager
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(QString("hardware"));
	s.remove("");
	s.beginWriteArray("instruments");
	for(int i=0;i<d_hardwareList.size();i++)
	{
		s.setArrayIndex(i);
        s.setValue(QString("key"),d_hardwareList.at(i).first->key());
	}
	s.endArray();
	s.endGroup();

	//now an array for all TCP instruments
	s.beginGroup(QString("tcp"));
	s.remove("");
	s.beginWriteArray("instruments");
	int index=0;
	for(int i=0;i<d_hardwareList.size();i++)
	{
        if(QString(d_hardwareList.at(i).first->metaObject()->superClass()->className()) == QString("TcpInstrument"))
		{
			s.setArrayIndex(index);
            s.setValue(QString("key"),d_hardwareList.at(i).first->key());
			index++;
		}
	}
	s.endArray();
	s.endGroup();

	//now an array for all RS232 instruments
	s.beginGroup(QString("rs232"));
	s.remove("");
	s.beginWriteArray("instruments");
	index=0;
	for(int i=0;i<d_hardwareList.size();i++)
	{
        if(QString(d_hardwareList.at(i).first->metaObject()->superClass()->className()) == QString("Rs232Instrument"))
		{
			s.setArrayIndex(index);
            s.setValue(QString("key"),d_hardwareList.at(i).first->key());
			index++;
		}
	}
	s.endArray();
	s.endGroup();
    s.sync();

    for(int i=0; i<d_hardwareList.size();i++)
    {
        QThread *thread = d_hardwareList.at(i).second;
        HardwareObject *obj = d_hardwareList.at(i).first;

        s.setValue(QString("%1/prettyName").arg(obj->key()),obj->name());
        s.setValue(QString("%1/connected").arg(obj->key()),false);

        connect(obj,&HardwareObject::logMessage,[=](QString msg, QtFTM::LogMessageCode mc){
            emit logMessage(QString("%1: %2").arg(obj->name()).arg(msg),mc);
        });
        connect(obj,&HardwareObject::connected,[=](bool success, QString msg){ connectionResult(obj,success,msg); });
        connect(obj,&HardwareObject::hardwareFailure,[=](){ hardwareFailure(obj); });
        if(thread != nullptr)
        {
            connect(thread,&QThread::started,obj,&HardwareObject::initialize);
            connect(thread,&QThread::finished,obj,&HardwareObject::deleteLater);
            obj->moveToThread(thread);
        }
        else
            obj->initialize();
    }

    //now, start all threads
    for(int i=0;i<d_hardwareList.size();i++)
    {
        QThread *thread = d_hardwareList.at(i).second;
        if(thread != nullptr)
        {
            if(!thread->isRunning())
                thread->start();
        }
    }



}

void HardwareManager::connectionResult(HardwareObject *obj, bool success, QString msg)
{
    //NOTE: flow controller failure is not a critical error. Allow program to operate even if flow controller does not connect.
	if(d_status.contains(obj->key()))
    {
        if(obj == fc)
            d_status[obj->key()] = true;
        else
            d_status[obj->key()] = success;
    }
	else
    {
        if(obj == fc)
            d_status.insert(obj->key(),true);
        else
            d_status.insert(obj->key(),success);
    }

	if(success)
		emit logMessage(obj->name().append(QString(" connected successfully.")));
	else
	{
		emit logMessage(obj->name().append(QString(" connection failed!")),QtFTM::LogError);
		emit logMessage(msg,QtFTM::LogError);
	}

	emit testComplete(obj->name(),success,msg);
	checkStatus();
}

void HardwareManager::testObjectConnection(QString type, QString key)
{
	if(type == QString("gpib"))
        QMetaObject::invokeMethod(gpib,"testObjectConnection",Q_ARG(QString,key));
	else
	{
		HardwareObject *obj = nullptr;
		for(int i=0; i<d_hardwareList.size();i++)
		{
            if(d_hardwareList.at(i).first->key() == key)
                obj = d_hardwareList.at(i).first;
		}
		if(!obj)
			emit testComplete(key,false,QString("Device not found!"));
		else
			QMetaObject::invokeMethod(obj,"testConnection");
	}
}

void HardwareManager::hardwareFailure(HardwareObject *obj)
{
    //re-test connection
    bool success = false;
    if(obj->thread() == thread())
        success = obj->testConnection();
    else
        QMetaObject::invokeMethod(obj,"testConnection",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,success));

    if(!success)
    {
        d_status[obj->key()] = false;
        if(!d_firstInitialization)
            emit failure();
    }
    else
        emit retryScan();

	checkStatus();
}

void HardwareManager::setFtmCavityFreqFromUI(double f)
{
    QMetaObject::invokeMethod(gpib,"setFtmCavityFreqFromUI",Q_ARG(double,f));
}

#ifndef CONFIG_NODRSYNTH
void HardwareManager::setDrSynthFreqFromUI(double f)
{
    QMetaObject::invokeMethod(gpib,"setDrSynthFreq",Q_ARG(double,f));
}

void HardwareManager::setDrSynthPwrFromUI(double p)
{
    QMetaObject::invokeMethod(gpib,"setDrSynthPower",Q_ARG(double,p));
}
#endif

void HardwareManager::setAttnFromUI(int a)
{
    attn->setAttn(a);
}


void HardwareManager::changeAttnFile(QString fileName)
{
    attn->changeAttenFile(fileName);
}

void HardwareManager::setPulse(const int ch, const PulseGenerator::Setting s, const QVariant x)
{
    QMetaObject::invokeMethod(pGen,"setChannelSetting",Q_ARG(int,ch),Q_ARG(PulseGenerator::Setting,s),Q_ARG(QVariant,x));
}

void HardwareManager::setPulse(const PulseGenerator::PulseChannelConfiguration p)
{
    QMetaObject::invokeMethod(pGen,"setChannelAll",Q_ARG(PulseGenerator::PulseChannelConfiguration,p));
}

QList<PulseGenerator::PulseChannelConfiguration> HardwareManager::readPGenAll()
{
	QList<PulseGenerator::PulseChannelConfiguration> out;
    QMetaObject::invokeMethod(pGen,"readAll",Qt::BlockingQueuedConnection,
						 Q_RETURN_ARG(QList<PulseGenerator::PulseChannelConfiguration>,out));
    return out;
}

QList<PulseGenerator::PulseChannelConfiguration> HardwareManager::configurePGenForTuning()
{
    QList<PulseGenerator::PulseChannelConfiguration> out;
    QMetaObject::invokeMethod(pGen,"configureForTuning",Qt::BlockingQueuedConnection,
                         Q_RETURN_ARG(QList<PulseGenerator::PulseChannelConfiguration>,out));
    return out;
}

QList<PulseGenerator::PulseChannelConfiguration> HardwareManager::setPulse(QList<PulseGenerator::PulseChannelConfiguration> l)
{
	QList<PulseGenerator::PulseChannelConfiguration> out;
    QMetaObject::invokeMethod(pGen,"setAll",Qt::BlockingQueuedConnection,
						 Q_RETURN_ARG(QList<PulseGenerator::PulseChannelConfiguration>,out),
						 Q_ARG(QList<PulseGenerator::PulseChannelConfiguration>,l));
	return out;
}

double HardwareManager::goToFtmSynthProbeFreq()
{
	double out = 0.0;
    QMetaObject::invokeMethod(gpib,"goToFtmProbeFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out));
	return out;
}

bool HardwareManager::goToFtmSynthCavityFreq()
{
	bool success = false;
    QMetaObject::invokeMethod(gpib,"goToFtmCavityFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,success));
	return success;
}

bool HardwareManager::setFtmSynthCavityFreq(double d)
{
    bool success = false;
    QMetaObject::invokeMethod(gpib,"setFtmCavityFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,success),Q_ARG(double,d));
    return success;
}

int HardwareManager::readCavityTuningVoltage()
{
    int out = -1;
    QMetaObject::invokeMethod(md,"lastTuneVoltage",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out));
    return out;
}

int HardwareManager::readTuneAttenuation()
{
    int out = -1;
    QMetaObject::invokeMethod(md,"lastTuneAttenuation",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out));
    return out;
}

int HardwareManager::setProtectionDelay(int a)
{
    int out = -1;
    QMetaObject::invokeMethod(pin,"setProtectionDelay",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out),Q_ARG(int, a));
    return out;
}

int HardwareManager::setScopeDelay(int a)
{
    int out = -1;
    QMetaObject::invokeMethod(pin,"setScopeDelay",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out),Q_ARG(int, a));
    return out;
}

int HardwareManager::readCalVoltage()
{
    int out = -1;
    QMetaObject::invokeMethod(md,"lastCalVoltage",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out));
    return out;
}

#ifndef CONFIG_NODRSYNTH
double HardwareManager::setDrSynthFreq(double f)
{
	double out = 0.0;
    QMetaObject::invokeMethod(gpib,"setDrSynthFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out),Q_ARG(double,f));
	return out;
}

double HardwareManager::setDrSynthPwr(double p)
{
	double out = 0.0;
    QMetaObject::invokeMethod(gpib,"setDrSynthPower",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out),Q_ARG(double,p));
    return out;
}
#endif

int HardwareManager::setCwMode(bool cw)
{
    long out = -1;
    QMetaObject::invokeMethod(iob,"cwMode",Qt::BlockingQueuedConnection,Q_RETURN_ARG(long,out),Q_ARG(bool,cw));
    return (int)out;
}

int HardwareManager::setMagnetMode(bool mag)
{
	long out = -1;
	QMetaObject::invokeMethod(iob,"setMagnet",Qt::BlockingQueuedConnection,Q_RETURN_ARG(long,out),Q_ARG(bool,mag));
	return out;
}

void HardwareManager::configureIOBoard()
{
    QMetaObject::invokeMethod(iob,"configure");
}

void HardwareManager::applyPGenSettings()
{
    QMetaObject::invokeMethod(pGen,"applySettings");
}

void HardwareManager::pauseScope(bool pause)
{
    QMetaObject::invokeMethod(scope,"setActive",Qt::BlockingQueuedConnection,Q_ARG(bool,!pause));
}

const FlowConfig HardwareManager::readFlowConfig()
{
    if(fc->thread() == thread())
        return fc->config();
    else
    {
        FlowConfig out;
        QMetaObject::invokeMethod(fc,"config",Qt::BlockingQueuedConnection,Q_RETURN_ARG(FlowConfig,out));
        return out;
    }
}

void HardwareManager::prepareForScan(Scan s)
{
	//the scan object contains the desired settings.
	//these will be sent to the hardware, and replaced with the actual values read from the hardware!
	//If there is a hardware failure, then emit the scanInitialized signal without calling Scan::initializationComplete
    //this will cause the scan to abort without saving, and will kill any batch process
    d_currentScan = s;
    if(s.tuningVoltageTakenWithScan())
    {

        bool skipTune = false;
        pauseScope(true);
        if(!d_currentScan.skipTune())
        {
            QMetaObject::invokeMethod(md,"canSkipTune",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,skipTune),Q_ARG(double,s.ftFreq()));
            if(skipTune)
                d_currentScan.setSkiptune(true);
        }

        d_waitingForScanTune = true;
        tuneCavity(d_currentScan.ftFreq(),-1,d_currentScan.skipTune());
    }
    else { // the equivalent of "emit"ing tuningComplete from motordriver  PRAA
        d_waitingForScanTune = true;    // this has to be set true or it won't call "finishPreparation
        cavityTuneComplete(true);   // this will set the tuning voltage by reading d_lastTuneVoltage from motor driver, which should be fine.
    }
}

void HardwareManager::finishPreparation(bool tuneSuccess)
{
    d_waitingForScanTune = false;
    if(!tuneSuccess)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }

    int v = readCavityTuningVoltage();
    if(v<0)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setTuningVoltage(v);

    // need this in dipole Moment calcs. Also used for cavity voltage
    int ta = readTuneAttenuation();
    double currentDipoleMoment = d_currentScan.dipoleMoment();
    int new_attenuation;
    if(currentDipoleMoment>=.01)
        new_attenuation =   qMax((int) round(  ((double) ta - 10.0 * log10((1.0/currentDipoleMoment) *( 1000.0/(double)v) ))), 0);
    else new_attenuation = d_currentScan.attenuation();
    //set attenuation for acquisition
    ///TODO: Use dipole moment and convert...
    int a = attn->setAttn(new_attenuation);
    if(a<0)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setAttenuation(a);

    // Calculate and save cavity voltage. Needs attenuation, tuning attenuation, and tuning voltage
    //double tuningVoltage,tuningAttenuation, currentAttenuation;// bubble,toil,trouble;
    double tvvalue,tavalue,cavalue,cvvalue;

    tvvalue= (double) v;
    tavalue= (double) ta;
    cavalue= (double) a;

   cvvalue= tvvalue*pow(10.0,(tavalue-cavalue)/10.0);

   if(cvvalue<0)
   {
       emit scanInitialized(d_currentScan);
       d_currentScan = Scan();
       pauseScope(false);
       return;
   }

    d_currentScan.setCavityVoltage( (int) cvvalue);


    // set current delays  into current scan object  -
    // first, get protection delay from either the pin module or the UI
    int pd = setProtectionDelay(d_currentScan.protectionDelayTime());
    if(pd<0)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setProtectionDelayTime( (int) pd);

    // then do the same thing for scope delay
    int sd = setScopeDelay(d_currentScan.scopeDelayTime());
    if(sd<0)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setScopeDelayTime( (int) sd);


    //set synthesizer to probe frequency
    double f = goToFtmSynthProbeFreq();
    if(f<0.0)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setProbeFreq(f);

#ifndef CONFIG_NODRSYNTH
    //set DR synthesizer
    f = setDrSynthFreq(d_currentScan.drFreq());
    if(f < 0.0)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setDrFreq(f);

    f = setDrSynthPwr(d_currentScan.drPower());
    if(f<-1e10)
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setDrPower(f);
#else
    d_currentScan.setDrFreq(-1.0);
    d_currentScan.setDrPower(0.0);
#endif

    //set pulse generator configuration
    QList<PulseGenerator::PulseChannelConfiguration> ls = setPulse(d_currentScan.pulseConfiguration());
    if(ls.isEmpty())
    {
        emit scanInitialized(d_currentScan);
        d_currentScan = Scan();
        pauseScope(false);
        return;
    }
    d_currentScan.setPulseConfiguration(ls);

    //set magnet mode
    int mag = setMagnetMode(d_currentScan.magnet());
    if(mag < 0)
    {
	    emit scanInitialized(d_currentScan);
	    d_currentScan = Scan();
	    pauseScope(false);
	    return;
    }
    d_currentScan.setMagnet(mag);

    //read flows and pressure
    FlowConfig c = readFlowConfig();
    d_currentScan.setFlowConfig(c);

    d_currentScan.initializationComplete();
    emit scanInitialized(d_currentScan);
    d_currentScan = Scan();
    pauseScope(false);

}

void HardwareManager::sleep(bool b)
{
	for(int i=0;i<d_hardwareList.size();i++)
    {
        if(d_status.value(d_hardwareList.at(i).first->key()))
            QMetaObject::invokeMethod(d_hardwareList.at(i).first,"sleep",Q_ARG(bool,b));
    }
}

void HardwareManager::scopeResolutionChanged()
{
    QMetaObject::invokeMethod(scope,"setResolution");
}

void HardwareManager::tuneCavity(double freq, int mode, bool measureOnly)
{
    emit statusMessage(QString("Tuning..."));
    //1.) Set synthesizer frequency to cavity f
    if(!setFtmSynthCavityFreq(freq))
    {
        emit statusMessage(QString("Tuning failed"));
        emit tuningComplete();
        if(d_waitingForScanTune)
            finishPreparation(false);
        return;
    }
    if(!goToFtmSynthCavityFreq())
    {
        emit statusMessage(QString("Tuning failed"));
        emit tuningComplete();
        if(d_waitingForScanTune)
            finishPreparation(false);
        return;
    }

    //2.) Set attenuation for tuning
    d_tuningOldA = attn->readAttn();
    int a = attn->setTuningAttn(freq);
    if(a<0)
    {
        emit statusMessage(QString("Tuning failed"));
        attn->setAttn(d_tuningOldA);
        goToFtmSynthProbeFreq();
        d_tuningOldA = -1;
        if(d_waitingForScanTune)
            finishPreparation(false);
        return;
    }

    //3.) Turn MW to DC (how?), turn off all other pulses except gas (high band?)
    if(d_tuningOldPulseConfig.isEmpty())
        d_tuningOldPulseConfig = configurePGenForTuning();
    else
        configurePGenForTuning();

    if(setCwMode(true) < 0)
    {
        emit statusMessage(QString("Tuning failed"));
        setPulse(d_tuningOldPulseConfig);
        attn->setAttn(d_tuningOldA);
        d_tuningOldPulseConfig.clear();
        d_tuningOldA = -1;
        goToFtmSynthProbeFreq();
        if(d_waitingForScanTune)
            finishPreparation(false);
        return;
    }

    //4.) Tune cavity
    if(!measureOnly)
    {
	    if(mode>0)
		    QMetaObject::invokeMethod(md,"tune",Q_ARG(double,freq),Q_ARG(int,a),Q_ARG(int,mode));
	    else
		    QMetaObject::invokeMethod(md,"tune",Q_ARG(double,freq),Q_ARG(int,a));
    }
    else
	    QMetaObject::invokeMethod(md,"measureVoltageNoTune");

}

void HardwareManager::cavityTuneComplete(bool success)
{
    //reconfigure everything
    setCwMode(false);

    //5-16-2014: More user-friendly to leave the attenuation at the tuning value in general
    //only reset original attenuation if we're not in a scan or a calibration
//    if(!d_waitingForScanTune && !d_waitingForCalibration)
//    {
//        if(d_tuningOldA >= 0)
//            attn->setAttn(d_tuningOldA);
//    }

    if(!d_tuningOldPulseConfig.isEmpty())
        setPulse(d_tuningOldPulseConfig);

    d_tuningOldPulseConfig.clear();
    d_tuningOldA = 0;

    goToFtmSynthProbeFreq();
    emit tuningComplete();

    //send appropriate status message
    QString msg("Tuning");
    if(d_waitingForCalibration)
        msg = QString("Calibration");


    if(success)
        emit statusMessage(QString("%1 successful!").arg(msg));
    else
        emit statusMessage(QString("%1 failed! See log for details.").arg(msg));

    d_waitingForCalibration = false;

    if(d_waitingForScanTune)
        finishPreparation(success);
}

void HardwareManager::calibrateCavity()
{
    emit statusMessage(QString("Calibrating..."));
    //1.) Set synthesizer frequency to cavity f
    if(!setFtmSynthCavityFreq(10030.0))
    {
        emit statusMessage(QString("Calibration unsuccessful."));
        emit tuningComplete();
        return;
    }

    if(!goToFtmSynthCavityFreq())
    {
        emit statusMessage(QString("Calibration unsuccessful."));
        emit tuningComplete();
        return;
    }

    //2.) Set attenuation for tuning
    int a = attn->setTuningAttn(10030.0);
    if(a<0)
    {
        emit statusMessage(QString("Calibration unsuccessful."));
        emit tuningComplete();
        return;
    }

    //3.) Turn MW to DC, turn off all other pulses except gas
    if(d_tuningOldPulseConfig.isEmpty())
        d_tuningOldPulseConfig = configurePGenForTuning();
    else
        configurePGenForTuning();

    if(setCwMode(true) < 0)
    {
        emit statusMessage(QString("Calibration unsuccessful."));
        setPulse(d_tuningOldPulseConfig);
        d_tuningOldPulseConfig.clear();
        emit tuningComplete();
        return;
    }

    d_waitingForCalibration = true;

    //4.) Tune cavity
    QMetaObject::invokeMethod(md,"calibrate");
}

void HardwareManager::changeCavityMode(double freq, bool above)
{
    int mode = -1;
    QMetaObject::invokeMethod(md,"calcNextMode",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,mode),Q_ARG(double,freq),Q_ARG(bool,above));
    if(mode>0)
        tuneCavity(freq,mode);
}

void HardwareManager::prepareForAttnTableGeneration(int a)
{
    //the goal is to determine the correct attenuation for mode 46 at 10 GHz, and calibrate the cavity
    //first, we need to reconfigure some things
    //we want to intercept the tune complete signal from the motor driver, and direct it to a different function
    //these will be reconnected when the preparation is complete
    disconnect(md,&MotorDriver::tuningComplete,this,&HardwareManager::cavityTuneComplete);
    connect(md,&MotorDriver::tuningComplete,this,&HardwareManager::attnTablePrepTuneComplete,Qt::UniqueConnection);

    attn->clearAttenData();
    d_tuningOldPulseConfig = readPGenAll();
    d_tuningOldA = attn->setAttn(a);
    d_waitingForCalibration = true;
    QMetaObject::invokeMethod(md,"shutUp",Q_ARG(bool,true));

    calibrateCavity();
}

void HardwareManager::attnTablePrepTuneComplete(bool success)
{
    //this function will be called when calibration or tuning finishes, so we need to act differently depending on what is going on
    //case 1: we're waiting for initial calibration at 20 dB, and need to adjust + tune
    //case 2: we've tuned, and now need to check if attenuation needs to be adjusted and then try again or calibrate
    //case 3: we've done the final calibration, and just need to make sure the voltage is legit

    int cv = 0;

    if(d_waitingForCalibration) //handle cases 1 and 3 here
    {
        d_waitingForCalibration = false;

        int v = readCalVoltage();
        cv = v;

        if(v <= 0 && !success) // something is very wrong... no choice but to abort unsuccessfully
        {
            emit logMessage(QString("Could not calibrate at %1 dB! Automatic attenuation table generation aborted.").arg(d_tuningOldA),QtFTM::LogError);
            restoreSettingsAfterAttnPrep(false);
            return;
        }

        //the saturated case and the too low case are handled outside this "if" block


        if(v > 500 && v < 2000) // here's case 3. We're done!
        {
            restoreSettingsAfterAttnPrep(true);
            return;
        }

    }

    //adjust attenuation and try again
    if(cv <=0)
        cv = readCavityTuningVoltage();

    if(cv > 3000) // voltage is saturated; increase by 10 dB and re-tune
    {
        int a = d_tuningOldA + 10;
        d_tuningOldA = attn->setAttn(a);
        tuneCavity(10030.0,46);
        return;
    }

    if(cv < 300) //voltage is too low to calculate reliably; try decreasing attenuation by 5 dB
    {
        if(!success && cv <= 0)
        {
            if(d_tuningOldA-5 < 0) // something is probably very wrong! Abort!
            {
                emit logMessage(QString("Could not tune at %1 dB! Automatic attenuation table generation aborted.").arg(d_tuningOldA),QtFTM::LogError);
                restoreSettingsAfterAttnPrep(false);
                return;
            }
        }
        int a = d_tuningOldA - 5;
        d_tuningOldA = attn->setAttn(a);
        tuneCavity(10030.0,46);
        return;
    }

    //if we've made it here, v >= 300 and v <= 3000. Calculate attenuation that gets us as close as possible to 1000 (max change is 5 dB, and recalibrate
    int newAttn = d_tuningOldA - (int)round(10.0*log10(1000.0/(double)cv));
    d_tuningOldA = attn->setAttn(newAttn);
    calibrateCavity();

}

void HardwareManager::restoreSettingsAfterAttnPrep(bool success)
{
    //restore configuration
    QMetaObject::invokeMethod(md,"shutUp",Q_ARG(bool,false));
    setCwMode(false);
    setPulse(d_tuningOldPulseConfig);
    d_tuningOldPulseConfig.clear();
    d_tuningOldA = 0;
    d_waitingForCalibration = false;
    disconnect(md,&MotorDriver::tuningComplete,this,&HardwareManager::attnTablePrepTuneComplete);
    connect(md,&MotorDriver::tuningComplete,this,&HardwareManager::cavityTuneComplete,Qt::UniqueConnection);
    emit attnTablePrepComplete(success);
}

void HardwareManager::checkStatus()
{
    //if numGpib is less than 0, the GPIB-LAN controller hasn't been initialized, so not everything is connected
    if(d_numGpib<0)
    {
//        emit allHardwareConnected(false);
        return;
    }

    //gotta wait until all instruments have responded
    if(d_status.size() < d_hardwareList.size() + d_numGpib)
    {
//        emit allHardwareConnected(false);
        return;
    }

	bool success = true;
	foreach (bool b, d_status)
	{
		if(!b)
			success = false;
	}

    if(success && d_firstInitialization)
    {
        //make sure nothing is asleep
        sleep(false);
        d_firstInitialization = false;
    }

	emit allHardwareConnected(success);
}

