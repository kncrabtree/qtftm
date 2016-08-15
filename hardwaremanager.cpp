#include "hardwaremanager.h"
#include <QTimer>
#include <QApplication>

HardwareManager::HardwareManager(QObject *parent) :
    QObject(parent), d_waitingForScanTune(false), d_waitingForCalibration(false), d_tuningOldA(-1), d_responseCount(0),
    d_firstInitialization(true), d_scanActive(false)
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
            if(p.second->isRunning())
            {
                p.second->quit();
                p.second->wait();
            }
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
    QThread *gpibThread = new QThread(this);
    d_hardwareList.append(qMakePair(gpib,gpibThread));

    p_ftmSynth = new FtmSynthHardware();
    connect(p_ftmSynth,&FtmSynthesizer::newCavityFreq,this,&HardwareManager::ftmSynthUpdate);
    connect(p_ftmSynth,&FtmSynthesizer::newProbeFreq,this,&HardwareManager::probeFreqUpdate);
    connect(p_ftmSynth,&FtmSynthesizer::rangeChanged,this,&HardwareManager::synthRangeChanged);
    connect(this,&HardwareManager::ftmSynthChangeBandFromUi,p_ftmSynth,&FtmSynthesizer::updateBandFromUi);
    connect(this,&HardwareManager::setFtmCavityFreqFromUI,p_ftmSynth,&FtmSynthesizer::setCavityFreqFromUI);
    d_hardwareList.append(qMakePair(p_ftmSynth,nullptr));

    p_drSynth = new DrSynthHardware();
    connect(p_drSynth,&DrSynthesizer::frequencyUpdate,this,&HardwareManager::drSynthFreqUpdate);
    connect(p_drSynth,&DrSynthesizer::powerUpdate,this,&HardwareManager::drSynthPwrUpdate);
    connect(p_drSynth,&DrSynthesizer::rangeChanged,this,&HardwareManager::synthRangeChanged);
    connect(this,&HardwareManager::drSynthChangeBandFromUi,p_drSynth,&DrSynthesizer::updateBandFromUi);
    connect(this,&HardwareManager::setDrSynthFreqFromUI,p_drSynth,&DrSynthesizer::setFreq);
    connect(this,&HardwareManager::setDrSynthPwrFromUI,p_drSynth,&DrSynthesizer::setPower);
    d_hardwareList.append(qMakePair(p_drSynth,nullptr));

    attn = new AttenuatorHardware();
	connect(attn,&Attenuator::attnUpdate,this,&HardwareManager::attenUpdate);
    connect(attn,&Attenuator::attenFileParseSuccess,this,&HardwareManager::attnLoadSuccess);
    connect(attn,&Attenuator::taattnUpdate,this,&HardwareManager::taattenUpdate);
    connect(this,&HardwareManager::setAttnFromUI,attn,&Attenuator::setAttn);
    d_hardwareList.append(qMakePair(attn,nullptr));

    pin = new PDGHardware();
    connect(pin,&PinSwitchDriveDelayGenerator::protectionDelayUpdate,this,&HardwareManager::hmProtectionDelayUpdate);
    connect(pin,&PinSwitchDriveDelayGenerator::scopeTriggerDelayUpdate,this,&HardwareManager::hmScopeDelayUpdate);
    connect(this,&HardwareManager::setProtectionDelayFromUI,pin,&PinSwitchDriveDelayGenerator::setProtectionDelay);
    connect(this,&HardwareManager::setScopeDelayFromUI,pin,&PinSwitchDriveDelayGenerator::setScopeDelay);
    d_hardwareList.append(qMakePair(pin,nullptr));

    md = new MotorDriverHardware();
    connect(md,&MotorDriver::posUpdate,this,&HardwareManager::mirrorPosUpdate);
    connect(md,&MotorDriver::tuningComplete,this,&HardwareManager::cavityTuneComplete);
    connect(p_ftmSynth,&FtmSynthesizer::newCavityFreq,md,&MotorDriver::cavityFreqChanged);
    connect(md,&MotorDriver::canTuneUp,this,&HardwareManager::canTuneUp);
    connect(md,&MotorDriver::canTuneDown,this,&HardwareManager::canTuneDown);
    connect(md,&MotorDriver::modeChanged,this,&HardwareManager::modeChanged);
    connect(md,&MotorDriver::voltageChanged,this,&HardwareManager::tuningVoltageChanged);
    connect(this,&HardwareManager::updateMotorSettings,md,&MotorDriver::readCavitySettings);
    d_hardwareList.append(qMakePair(md,nullptr));

    iob = new IOBoardHardware();
    connect(iob,&IOBoard::triggered,scope,&Oscilloscope::sendCurveQuery);
    connect(iob,&IOBoard::magnetUpdate,this,&HardwareManager::magnetUpdate);
    connect(this,&HardwareManager::setMagnetFromUI,iob,&IOBoard::setMagnet);
    d_hardwareList.append(qMakePair(iob,nullptr));


    fc = new FlowControllerHardware();
    connect(fc,&FlowController::flowUpdate,this,&HardwareManager::flowUpdate);
    connect(fc,&FlowController::pressureUpdate,this,&HardwareManager::pressureUpdate);
    connect(fc,&FlowController::flowSetpointUpdate,this,&HardwareManager::flowSetpointUpdate);
    connect(fc,&FlowController::pressureSetpointUpdate,this,&HardwareManager::pressureSetpointUpdate);
    connect(fc,&FlowController::pressureControlMode,this,&HardwareManager::pressureControlMode);
    connect(this,&HardwareManager::setPressureControlMode,fc,&FlowController::setPressureControlMode);
    connect(this,&HardwareManager::setGasName,fc,&FlowController::setChannelName);
    connect(this,&HardwareManager::setFlowSetpoint,fc,&FlowController::setFlowSetpoint);
    connect(this,&HardwareManager::setPressureSetpoint,fc,&FlowController::setPressureSetpoint);
    d_hardwareList.append(qMakePair(fc,nullptr));

    pGen = new PulseGeneratorHardware();
    connect(pGen,&PulseGenerator::settingUpdate,this,&HardwareManager::pGenChannelSetting);
    connect(pGen,&PulseGenerator::configUpdate,this,&HardwareManager::pGenConfigUpdate);
    connect(this,&HardwareManager::setRepRate,pGen,&PulseGenerator::setRepRate);
    connect(pGen,&PulseGenerator::repRateUpdate,this,&HardwareManager::repRateUpdate);
    connect(this,&HardwareManager::setPulseSetting,pGen,&PulseGenerator::set);
    d_hardwareList.append(qMakePair(pGen,nullptr));

    p_hvps = new HvPowerSupplyHardware();
    connect(this,&HardwareManager::setDcVoltageFromUI,p_hvps,&HvPowerSupply::setVoltage);
    connect(p_hvps,&HvPowerSupply::voltageUpdate,this,&HardwareManager::dcVoltageUpdate);
    d_hardwareList.append(qMakePair(p_hvps,nullptr));

	//write arrays of the connected devices for use in the Hardware Settings menu
	//first array is for all objects accessible to the hardware manager
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(QString("hardware"));
	s.remove("");
	s.beginWriteArray("instruments");
	for(int i=0;i<d_hardwareList.size();i++)
	{
		HardwareObject *obj = d_hardwareList.at(i).first;
		s.setArrayIndex(i);
		s.setValue(QString("key"),obj->key());
		s.setValue(QString("subKey"),obj->subKey());
		s.setValue(QString("prettyName"),obj->name());
		s.setValue(QString("critical"),obj->isCritical());
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
	   if(d_hardwareList.at(i).first->type() == CommunicationProtocol::Tcp)
		{
			s.setArrayIndex(index);
			s.setValue(QString("key"),d_hardwareList.at(i).first->key());
			s.setValue(QString("subKey"),d_hardwareList.at(i).first->subKey());
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
	   if(d_hardwareList.at(i).first->type() == CommunicationProtocol::Rs232)
		{
		   s.setArrayIndex(index);
		   s.setValue(QString("key"),d_hardwareList.at(i).first->key());
		   s.setValue(QString("subKey"),d_hardwareList.at(i).first->subKey());
		   index++;
		}
	}
	s.endArray();
	s.endGroup();

	//now an array for all GPIB instruments
	s.beginGroup(QString("gpib"));
	s.remove("");
	s.beginWriteArray("instruments");
	index=0;
	for(int i=0;i<d_hardwareList.size();i++)
	{
		if(d_hardwareList.at(i).first->type() == CommunicationProtocol::Gpib)
		{
			s.setArrayIndex(index);
			s.setValue(QString("key"),d_hardwareList.at(i).first->key());
			s.setValue(QString("subKey"),d_hardwareList.at(i).first->subKey());
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
        if(obj->isThreaded())
        {
            if(obj != gpib)
            {
                if(obj->type() != CommunicationProtocol::Gpib)
                    thread = new QThread(this);
                else
                    thread = gpibThread;

                d_hardwareList[i].second = thread;
            }
        }

        //note: gpib will only be used if the device is a GpibInstrument
        obj->buildCommunication(gpib);

        s.setValue(QString("%1/prettyName").arg(obj->key()),obj->name());
        s.setValue(QString("%1/subKey").arg(obj->key()),obj->subKey());
        s.setValue(QString("%1/connected").arg(obj->key()),false);
        s.setValue(QString("%1/critical").arg(obj->key()),obj->isCritical());

        connect(obj,&HardwareObject::logMessage,[=](QString msg, QtFTM::LogMessageCode mc){
            emit logMessage(QString("%1: %2").arg(obj->name()).arg(msg),mc);
        });
        connect(obj,&HardwareObject::connected,[=](bool success, QString msg){ connectionResult(obj,success,msg); });
        if(thread != nullptr)
        {
            connect(thread,&QThread::started,obj,&HardwareObject::initialize);
            connect(thread,&QThread::finished,obj,&HardwareObject::deleteLater);
            if(obj->parent() == nullptr)
                obj->moveToThread(thread);
        }
        else
            obj->initialize();
    }

    //final connections that might be blocking depending on threading
    if(p_ftmSynth->thread() != iob->thread())
        connect(p_ftmSynth,&FtmSynthesizer::triggerBandChange,iob,&IOBoard::ftmSynthBand,Qt::BlockingQueuedConnection);
    else
        connect(p_ftmSynth,&FtmSynthesizer::triggerBandChange,iob,&IOBoard::ftmSynthBand);
    if(p_ftmSynth->thread() != md->thread())
        connect(md,&MotorDriver::deltaF,p_ftmSynth,&FtmSynthesizer::goToCavityDeltaFreq,Qt::BlockingQueuedConnection);
    else
        connect(md,&MotorDriver::deltaF,p_ftmSynth,&FtmSynthesizer::goToCavityDeltaFreq);

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
	if(d_responseCount < d_hardwareList.size())
		d_responseCount++;

	if(success)
	{
		connect(obj,&HardwareObject::hardwareFailure,this,&HardwareManager::hardwareFailure,Qt::UniqueConnection);
		emit logMessage(obj->name().append(QString(": Connected successfully.")));
		if(obj->type() == CommunicationProtocol::Virtual)
			emit logMessage(QString("%1 is a virtual instrument. Be cautious about taking real measurements!")
						 .arg(obj->name()),QtFTM::LogWarning);
	}
	else
	{
		disconnect(obj,&HardwareObject::hardwareFailure,this,&HardwareManager::hardwareFailure);
		emit logMessage(obj->name().append(QString(": Connection failed!")),QtFTM::LogError);
		if(!msg.isEmpty())
			emit logMessage(QString("%1: %2").arg(obj->name()).arg(msg),QtFTM::LogError);
	}

	obj->setConnected(success);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.setValue(QString("%1/connected").arg(obj->key()),success);
	s.sync();

	emit testComplete(obj->name(),success,msg);
	checkStatus();
}

void HardwareManager::testObjectConnection(QString type, QString key)
{
	Q_UNUSED(type)
	HardwareObject *obj = nullptr;
	for(int i=0; i<d_hardwareList.size();i++)
	{
	    if(d_hardwareList.at(i).first->key() == key)
		   obj = d_hardwareList.at(i).first;
	}
	if(obj == nullptr)
	    emit testComplete(key,false,QString("Device not found!"));
	else
	{
		if(obj->thread() == thread())
			obj->testConnection();
		else
			QMetaObject::invokeMethod(obj,"testConnection");
	}

}

void HardwareManager::hardwareFailure()
{	
	HardwareObject *obj = dynamic_cast<HardwareObject*>(sender());
	if(obj == nullptr)
		return;

	disconnect(obj,&HardwareObject::hardwareFailure,this,&HardwareManager::hardwareFailure);
    bool success = false;
    emit logMessage(QString("%1: Hardware failure detected. Automatically retrying connection...").arg(obj->name()),QtFTM::LogWarning);
    if(obj->thread() == thread())
        success = obj->testConnection();
    else
        QMetaObject::invokeMethod(obj,"testConnection",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,success));

    if(d_scanActive)
    {
	    if(!success)
		    emit failure();
	    else
		    emit retryScan();
    }

    //checkStatus will be called when the connectionResult slot is invoked
}

void HardwareManager::changeAttnFile(QString fileName)
{
	if(attn->thread() == thread())
		attn->changeAttenFile(fileName);
	else
		QMetaObject::invokeMethod(attn,"changeAttenFile",Q_ARG(QString,fileName));
}

PulseGenConfig HardwareManager::configurePGenForTuning()
{
	if(pGen->thread() == thread())
		return pGen->configureForTuning();
	else
	{
		PulseGenConfig out;
		QMetaObject::invokeMethod(pGen,"configureForTuning",Qt::BlockingQueuedConnection,
							 Q_RETURN_ARG(PulseGenConfig,out));
		return out;
	}
}

PulseGenConfig HardwareManager::setPulseConfig(const PulseGenConfig c)
{
	if(pGen->thread() == thread())
	{
		bool success = pGen->setAll(c);
		if(success)
			return pGen->config();
		else
			return PulseGenConfig();
	}
	else
	{
		bool success = false;
		PulseGenConfig out;
		QMetaObject::invokeMethod(pGen,"setAll",Qt::BlockingQueuedConnection,
							 Q_RETURN_ARG(bool,success),Q_ARG(PulseGenConfig,c));
		if(success)
			QMetaObject::invokeMethod(pGen,"config",Qt::BlockingQueuedConnection,Q_RETURN_ARG(PulseGenConfig,out));

		return out;
	}
}

double HardwareManager::goToFtmSynthProbeFreq()
{
	if(p_ftmSynth->thread() == thread())
		return p_ftmSynth->goToProbeFreq();
	else
	{
		double out = 0.0;
		QMetaObject::invokeMethod(p_ftmSynth,"goToProbeFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out));
		return out;
	}
}

bool HardwareManager::goToFtmSynthCavityFreq()
{
	if(p_ftmSynth->thread() == thread())
		return p_ftmSynth->goToCavityFreq();
	else
	{
		bool success = false;
		QMetaObject::invokeMethod(p_ftmSynth,"goToCavityFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,success));
		return success;
	}
}

bool HardwareManager::setFtmSynthCavityFreq(double d)
{
	if(p_ftmSynth->thread() == thread())
		return p_ftmSynth->setCavityFreq(d);
	else
	{
		bool success = false;
		QMetaObject::invokeMethod(p_ftmSynth,"setCavityFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,success),Q_ARG(double,d));
		return success;
	}
}

void HardwareManager::startTune(double freq, int attn, int mode)
{
	if(md->thread() == thread())
		md->tune(freq,attn,mode);
	else
		QMetaObject::invokeMethod(md,"tune",Q_ARG(double,freq),Q_ARG(int,attn),Q_ARG(int,mode));
}

int HardwareManager::measureCavityVoltage()
{
	if(md->thread() == thread())
		return md->measureVoltageNoTune();
	else
	{
		int out;
		QMetaObject::invokeMethod(md,"measureVoltageNoTune",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out));
		return out;
	}
}

void HardwareManager::startCalibration()
{
	if(md->thread() == thread())
		md->calibrate();
	else
		QMetaObject::invokeMethod(md,"calibrate");
}

void HardwareManager::shutUpMotorDriver(bool quiet)
{
	if(md->thread() == thread())
		md->shutUp(quiet);
	else
		QMetaObject::invokeMethod(md,"shutUp",Qt::BlockingQueuedConnection,Q_ARG(bool,quiet));
}

int HardwareManager::readCavityTuningVoltage()
{
	if(md->thread() == thread())
		return md->lastTuneVoltage();
	else
	{
		int out = -1;
		QMetaObject::invokeMethod(md,"lastTuneVoltage",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out));
		return out;
	}
}

int HardwareManager::readTuneAttenuation()
{
	if(md->thread() == thread())
		return md->lastTuneAttenuation();
	else
	{
		int out = -1;
		QMetaObject::invokeMethod(md,"lastTuneAttenuation",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out));
		return out;
	}
}

int HardwareManager::setProtectionDelay(int a)
{
	if(pin->thread() == thread())
		return pin->setProtectionDelay(a);
	else
	{
		int out = -1;
		QMetaObject::invokeMethod(pin,"setProtectionDelay",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out),Q_ARG(int, a));
		return out;
	}
}

int HardwareManager::setScopeDelay(int a)
{
	if(pin->thread() == thread())
		return pin->setScopeDelay(a);
	else
	{
		int out = -1;
		QMetaObject::invokeMethod(pin,"setScopeDelay",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out),Q_ARG(int, a));
		return out;
	}
}

int HardwareManager::setDcVoltage(int a)
{
	if(p_hvps->thread() == thread())
		return p_hvps->setVoltage(a);
	else
	{
		int out;
		QMetaObject::invokeMethod(p_hvps,"setVoltage",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out),Q_ARG(int,a));
		return out;
	}
}

int HardwareManager::readCalVoltage()
{
	if(md->thread() == thread())
		return md->lastCalVoltage();
	else
	{
		int out = -1;
		QMetaObject::invokeMethod(md,"lastCalVoltage",Qt::BlockingQueuedConnection,Q_RETURN_ARG(int,out));
		return out;
	}
}

double HardwareManager::setDrSynthFreq(double f)
{
	if(p_drSynth->thread() == thread())
		return p_drSynth->setFreq(f);
	else
	{
		double out = 0.0;
		QMetaObject::invokeMethod(p_drSynth,"setFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out),Q_ARG(double,f));
		return out;
	}
}

double HardwareManager::setDrSynthPwr(double p)
{
	if(p_drSynth->thread() == thread())
		return p_drSynth->setPower(p);
	else
	{
		double out = 0.0;
		QMetaObject::invokeMethod(p_drSynth,"setPower",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out),Q_ARG(double,p));
		return out;
    }
}

double HardwareManager::readDrFrequency()
{
    if(p_drSynth->thread() == thread())
        return p_drSynth->readFreq();
    else
    {
        double out = 0.0;
        QMetaObject::invokeMethod(p_drSynth,"readFreq",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out));
        return out;
    }
}

double HardwareManager::readDrPower()
{
    if(p_drSynth->thread() == thread())
        return p_drSynth->readPower();
    else
    {
        double out = 0.0;
        QMetaObject::invokeMethod(p_drSynth,"readPower",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out));
        return out;
    }
}

int HardwareManager::setCwMode(bool cw)
{
	if(iob->thread() == thread())
		return iob->setCwMode(cw);
	else
	{
		long out = -1;
		QMetaObject::invokeMethod(iob,"setCwMode",Qt::BlockingQueuedConnection,Q_RETURN_ARG(long,out),Q_ARG(bool,cw));
		return (int)out;
	}
}

int HardwareManager::setMagnetMode(bool mag)
{
	if(iob->thread() == thread())
		return iob->setMagnet(mag);
	else
	{
		long out = -1;
		QMetaObject::invokeMethod(iob,"setMagnet",Qt::BlockingQueuedConnection,Q_RETURN_ARG(long,out),Q_ARG(bool,mag));
		return out;
	}
}

void HardwareManager::pauseScope(bool pause)
{
	if(scope->thread() == this->thread())
		scope->setActive(!pause);
	else
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
    d_waitingForScanTune = true;
    d_scanActive = true;
    pauseScope(true);

    if(d_currentScan.skipTune())
	    finishPreparation(true);
    else
    {
        //for batch attenuation scans, need to set attenuation before tuning
        //TODO: write wrapper function for setAttn to handle threading
        if(s.isDummy())
            attn->setAttn(s.attenuation());
	    tuneCavity(d_currentScan.ftFreq(),-1);
    }

}

void HardwareManager::finishPreparation(bool tuneSuccess)
{
    d_waitingForScanTune = false;
    bool success = true;

    if(tuneSuccess)
    {
	    int v = readCavityTuningVoltage();
        if(v>=0)
	    {
		    d_currentScan.setTuningVoltage(v);

		    // need this in dipole Moment calcs. Also used for cavity voltage
		    int ta = readTuneAttenuation();
		    if(ta < 0)
			    success = false;
		    else
		    {
			    double tvvalue = static_cast<double>(v);
			    double tavalue = static_cast<double>(ta);

			    double currentDipoleMoment = d_currentScan.dipoleMoment();
			    int new_attenuation = d_currentScan.attenuation();
			    if(currentDipoleMoment > .005)
				    new_attenuation = qMax(qRound(tavalue - 10.0*log10((1.0/currentDipoleMoment)*(1000.0/tvvalue))),0);
			    //set attenuation for acquisition
			    int a = attn->setAttn(new_attenuation);
			    if(a<0)
				    success = false;
			    else
			    {
				    d_currentScan.setAttenuation(a);
				    double cavalue = static_cast<double>(a);
				    double cvvalue = qMax(-1.0,tvvalue*pow(10.0,(tavalue-cavalue)/10.0));

				    //don't abort if cavity voltage < 0
				    //in a batch, it will be matked as a bad tune
				    d_currentScan.setCavityVoltage(qRound(cvvalue));
			    }
		    }
	    }
    } //if tuning was not successful, leave attenuation at tuning attn


    if(success)
    {
	    //set dc voltage, but only if connected
	    if(p_hvps->isConnected())
	    {
		    int dc = setDcVoltage(d_currentScan.dcVoltage());
		    if(dc < 0)
			    success = false;
		    else
			    d_currentScan.setDcVoltage(dc);
	    }
    }

    if(success)
    {
	    // set current delays  into current scan object  -
	    // first, get protection delay from either the pin module or the UI
	    int pd = setProtectionDelay(d_currentScan.protectionDelayTime());
	    if(pd<0)
		    success = false;
	    else
		    d_currentScan.setProtectionDelayTime(pd);
    }

    if(success)
    {
	    // then do the same thing for scope delay
	    int sd = setScopeDelay(d_currentScan.scopeDelayTime());
	    if(sd<0)
		    success = false;
	    else
		    d_currentScan.setScopeDelayTime(sd);
    }


    if(success)
    {
	    //set synthesizer to probe frequency
	    double f = goToFtmSynthProbeFreq();
	    if(f<0.0)
		    success = false;
	    else
		    d_currentScan.setProbeFreq(f);
    }

    if(success)
    {
	    //set DR synthesizer
	    double f = setDrSynthFreq(d_currentScan.drFreq());
	    if(f < 0.0)
		    success = false;
	    else
		    d_currentScan.setDrFreq(f);
    }

    if(success)
    {
	    double p = setDrSynthPwr(d_currentScan.drPower());
	    if(p<-1e10)
		    success = false;
	    else
		    d_currentScan.setDrPower(p);
    }

    if(success)
    {
	    //set pulse generator configuration
        //don't change gas pulse length
	    PulseGenConfig pc = setPulseConfig(d_currentScan.pulseConfiguration());
	    if(pc.isEmpty())
		    success = false;
	    else
		    d_currentScan.setPulseConfiguration(pc);
    }

    //set magnet mode
    if(success)
    {
	    int mag = setMagnetMode(d_currentScan.magnet());
	    if(mag < 0)
		    success = false;
	    else
		    d_currentScan.setMagnet(mag);
    }

    //read flows and pressure
    FlowConfig c = readFlowConfig();
    d_currentScan.setFlowConfig(c);

    if(success)
    {
	    d_currentScan.initializationComplete();
    }

    emit scanInitialized(d_currentScan);

    restoreSettingsAfterScanPrep();

}

void HardwareManager::sleep(bool b)
{
	for(int i=0;i<d_hardwareList.size();i++)
    {
		HardwareObject *obj = d_hardwareList.at(i).first;
		if(obj->isConnected())
		{
			if(obj->thread() == thread())
				obj->sleep(b);
			else
				QMetaObject::invokeMethod(d_hardwareList.at(i).first,"sleep",Q_ARG(bool,b));
		}
    }
}

void HardwareManager::scopeResolutionChanged()
{
    if(scope->thread() == thread())
        scope->setResolution();
    else
        QMetaObject::invokeMethod(scope,"setResolution");
}

void HardwareManager::tuneCavity(double freq, int mode)
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
    if(d_tuningOldA < 0)
    {
	    emit statusMessage(QString("Tuning failed"));
	    goToFtmSynthProbeFreq();
	    d_tuningOldA = -1;
	    if(d_waitingForScanTune)
		   finishPreparation(false);
	    return;
    }

    int a = attn->setTuningAttn(freq);
    if(a<0)
    {
        emit statusMessage(QString("Tuning failed"));
        goToFtmSynthProbeFreq();
        d_tuningOldA = -1;
        if(d_waitingForScanTune)
            finishPreparation(false);
        return;
    }

    //3.) Turn MW to DC (how?), turn off all other pulses except gas (high band?)
    d_tuningOldPulseConfig = configurePGenForTuning();

    double drf = readDrFrequency();
    double drp = readDrPower();
    d_tuningOldDr = qMakePair(drf,drp);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("drSynth"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    double minDr = s.value(QString("min")).toDouble();
    double minPwr = s.value(QString("minPower"),-20.0).toDouble();
    minDr += 648.21;
    s.endGroup();
    s.endGroup();

    setDrSynthFreq(minDr);
    setDrSynthPwr(minPwr);



    if(setCwMode(true) < 0)
    {
        emit statusMessage(QString("Tuning failed"));
	   setPulseConfig(d_tuningOldPulseConfig);
        attn->setAttn(d_tuningOldA);
        d_tuningOldA = -1;
        goToFtmSynthProbeFreq();
        if(d_waitingForScanTune)
            finishPreparation(false);
        return;
    }

    //4.) Tune cavity
    startTune(freq,a,mode);

}

void HardwareManager::cavityTuneComplete(bool success)
{
    //reconfigure everything
    setCwMode(false);

    if(!d_tuningOldPulseConfig.isEmpty())
	   setPulseConfig(d_tuningOldPulseConfig);

    d_tuningOldA = 0;

    goToFtmSynthProbeFreq();
    emit tuningComplete();

    setDrSynthFreq(d_tuningOldDr.first);
    setDrSynthPwr(d_tuningOldDr.second);
    d_tuningOldDr = qMakePair(0.0,0.0);

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
    d_tuningOldPulseConfig = configurePGenForTuning();

    if(setCwMode(true) < 0)
    {
	    emit statusMessage(QString("Calibration unsuccessful."));
	    if(!d_tuningOldPulseConfig.isEmpty())
		    setPulseConfig(d_tuningOldPulseConfig);
	    emit tuningComplete();
	    return;
    }

    d_waitingForCalibration = true;

    //4.) Tune cavity
    startCalibration();
}

void HardwareManager::changeCavityMode(double freq, bool above)
{
	int mode = -1;
	if(md->thread() == thread())
		mode = md->calcNextMode(freq,above);
	else
		QMetaObject::invokeMethod(md,"calcNextMode",Qt::BlockingQueuedConnection,
							 Q_RETURN_ARG(int,mode),Q_ARG(double,freq),Q_ARG(bool,above));

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
    d_tuningOldA = attn->setAttn(a);
    d_waitingForCalibration = true;
    shutUpMotorDriver(true);

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
    shutUpMotorDriver(false);
    setCwMode(false);
    if(!d_tuningOldPulseConfig.isEmpty())
	    setPulseConfig(d_tuningOldPulseConfig);
    d_tuningOldA = 0;
    d_waitingForCalibration = false;
    disconnect(md,&MotorDriver::tuningComplete,this,&HardwareManager::attnTablePrepTuneComplete);
    connect(md,&MotorDriver::tuningComplete,this,&HardwareManager::cavityTuneComplete,Qt::UniqueConnection);
    emit attnTablePrepComplete(success);
}

void HardwareManager::restoreSettingsAfterScanPrep()
{
	d_tuningOldA = 0;
	d_tuningOldPulseConfig = PulseGenConfig();
	pauseScope(false);
	d_currentScan = Scan();
	return;
}

void HardwareManager::scanComplete(const Scan s)
{
	Q_UNUSED(s)
	d_scanActive = false;
}

void HardwareManager::checkStatus()
{
	//gotta wait until all instruments have responded
	if(d_responseCount < d_hardwareList.size())
	    return;

	bool success = true;

	for(int i=0; i<d_hardwareList.size(); i++)
	{
		HardwareObject *obj = d_hardwareList.at(i).first;
		if(!obj->isConnected() && obj->isCritical())
			success = false;
	}

	emit allHardwareConnected(success);
}

