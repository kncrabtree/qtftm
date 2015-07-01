#include "gpiblancontroller.h"
#include <QApplication>

GpibLanController::GpibLanController(QObject *parent) :
	TcpInstrument(QString("gpiblan"),QString("GPIB-LAN Controller"),parent), d_currentAddress(0)
{
}


void GpibLanController::initialize()
{
	QMutexLocker l(&mutex);
	TcpInstrument::initialize();

	d_ftmSynth = new FtmSynthesizer(d_socket,this);
	connect(d_ftmSynth,&HardwareObject::logMessage,this,&HardwareObject::logMessage);
	connect(d_ftmSynth,&FtmSynthesizer::newFreq,this,&GpibLanController::ftmSynthFreq);
	connect(d_ftmSynth,&FtmSynthesizer::newProbeFreq,this,&GpibLanController::probeFreqUpdate);
    connect(d_ftmSynth,&HardwareObject::connected,this,&HardwareObject::connected);
	connect(d_ftmSynth,&HardwareObject::hardwareFailure,this,&HardwareObject::hardwareFailure);
    connect(d_ftmSynth,&Synthesizer::rangeChanged,this,&GpibLanController::rangeChanged);
    connect(d_ftmSynth,&Synthesizer::triggerBandChange,this,&GpibLanController::ftmSynthBandChanged);
	d_ftmSynth->initialize();
	d_instruments.append(d_ftmSynth);

#ifndef CONFIG_NODRSYNTH
	d_drSynth = new DrSynthesizer(d_socket,this);
	connect(d_drSynth,&HardwareObject::logMessage,this,&HardwareObject::logMessage);
	connect(d_drSynth,&DrSynthesizer::newFreq,this,&GpibLanController::drSynthFreq);
	connect(d_drSynth,&DrSynthesizer::newPower,this,&GpibLanController::drSynthPower);
    connect(d_drSynth,&HardwareObject::connected,this,&HardwareObject::connected);
	connect(d_drSynth,&HardwareObject::hardwareFailure,this,&HardwareObject::hardwareFailure);
    connect(d_drSynth,&Synthesizer::rangeChanged,this,&GpibLanController::rangeChanged);
    connect(d_drSynth,&Synthesizer::triggerBandChange,this,&GpibLanController::drSynthBandChanged);
	d_drSynth->initialize();
	d_instruments.append(d_drSynth);
#endif	

	//write an array of the connected GPIB devices for use in the Hardware Settings menu
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(QString("gpib"));
	s.remove("");
	s.beginWriteArray("instruments");
	for(int i=0;i<d_instruments.size();i++)
	{
		s.setArrayIndex(i);
		s.setValue(QString("key"),d_instruments.at(i)->key());
	}
	s.endArray();
	s.endGroup();
	s.sync();
    emit numInstruments(d_instruments.size());

	l.unlock();
    if(testConnection())
    {
        l.relock();
        for(int i=0;i<d_instruments.size();i++)
        {
            if(setAddress(d_instruments.at(i)->address()))
                d_instruments.at(i)->testConnection();
            else
                break;
        }
    }
    else
    {
        for(int i=0;i<d_instruments.size();i++)
            emit connected(false,QString("Could not connect because %1 connection failed.").arg(d_prettyName));
    }

}

bool GpibLanController::testConnection()
{

	//test communication with controller here, and get current address!
	QMutexLocker l(&mutex);

    if(!TcpInstrument::testConnection())
    {
        emit connected(false,QString("TCP Socket error"));
        return false;
    }

	//if success, do some specific test for this device
    QByteArray resp = queryCmd(QString("++ver\n"));
    if(resp.isEmpty())
    {
        emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
        return false;
    }
    if(!resp.startsWith("Prologix GPIB-ETHERNET Controller"))
    {
        emit connected(false,QString("%1 response invalid. Received: %2").arg(d_prettyName).arg(QString(resp)));
        return false;
    }

    emit logMessage(QString("%1 ID response: %2").arg(d_prettyName).arg(QString(resp)));

    writeCmd(QString("++auto 0\n"));
    writeCmd(QString("++savecfg 0\n"));
    writeCmd(QString("++read_tmo_ms 50\n"));

    d_currentAddress = queryCmd(QString("++addr\n")).trimmed().toInt();
    emit logMessage(QString("%1 Current address: %2").arg(d_prettyName).arg(d_currentAddress));
    emit connected();
    return true;
}

void GpibLanController::testObjectConnection(QString key)
{
	QMutexLocker l(&mutex);
	GpibInstrument *obj = nullptr;
	for(int i=0;i<d_instruments.size();i++)
	{
		if(d_instruments.at(i)->key() == key)
			obj = d_instruments.at(i);
	}
	if(!obj)
		emit testComplete(key,false,QString("Device not found!"));
	else
    {
        if(setAddress(obj->address()))
            obj->testConnection();
    }
}

void GpibLanController::sleep(bool b)
{
	QMutexLocker l(&mutex);
	for(int i=0;i<d_instruments.size();i++)
	{
        if(setAddress(d_instruments.at(i)->address()))
            d_instruments.at(i)->sleep(b);
    }
}

void GpibLanController::setFtmCavityFreqFromUI(double f)
{
	QMutexLocker l(&mutex);
	setAddress(d_ftmSynth->address());
    d_ftmSynth->setCavityFreqFromUI(f);
}

void GpibLanController::ftmDeltaFreq(double delta)
{
    QMutexLocker l(&mutex);
    setAddress(d_ftmSynth->address());
    d_ftmSynth->goToCavityDeltaFreq(delta);
}

#ifndef CONFIG_NODRSYNTH
double GpibLanController::setDrSynthFreq(double f)
{
	QMutexLocker l(&mutex);
	setAddress(d_drSynth->address());
	return d_drSynth->setFreq(f);
}

double GpibLanController::setDrSynthPower(double p)
{
	QMutexLocker l(&mutex);
	setAddress(d_drSynth->address());
    return d_drSynth->setPower(p);
}
#endif

void GpibLanController::ftmSynthChangeBandFromUi()
{
    QMutexLocker l(&mutex);
    setAddress(d_ftmSynth->address());
    d_ftmSynth->updateBandFromUi();
}

#ifndef CONFIG_NODRSYNTH
void GpibLanController::drSynthChangeBandFromUi()
{
    QMutexLocker l(&mutex);
    setAddress(d_drSynth->address());
    d_drSynth->updateBandFromUi();
}
#endif

double GpibLanController::goToFtmProbeFreq()
{
	QMutexLocker l(&mutex);
	setAddress(d_ftmSynth->address());
	return d_ftmSynth->goToProbeFreq();
}

bool GpibLanController::goToFtmCavityFreq()
{
	QMutexLocker l(&mutex);
	setAddress(d_ftmSynth->address());
    return d_ftmSynth->goToCavityFreq();
}

bool GpibLanController::setFtmCavityFreq(double d)
{
	QMutexLocker l(&mutex);
	setAddress(d_ftmSynth->address());
    return d_ftmSynth->setCavityFreq(d);
}


bool GpibLanController::setAddress(int a)
{
	if(d_socket->state() != QTcpSocket::ConnectedState)
	{
	    if(!connectSocket())
	    {
		   emit hardwareFailure();
		   emit logMessage(QString("Could not connect to %1. Socket is not connected.").arg(d_prettyName));
		   return false;
	    }
	}

    if(d_socket->bytesAvailable())
        d_socket->readAll();

	//write address command if d_currentAddress != a
    if(d_currentAddress != a)
    {
        if(!writeCmd(QString("++addr %1\n").arg(a)))
        {
            d_socket->waitForReadyRead(100);
            if(d_socket->bytesAvailable())
                d_socket->readAll();

            if(!writeCmd(QString("++addr %1\n").arg(a)))
            {
                emit hardwareFailure();
                emit logMessage(QString("Could not set %1 address to %2").arg(d_prettyName).arg(a),LogHandler::Error);
                return false;
            }
        }
        if(d_socket->bytesAvailable())
            d_socket->readAll();
        bool ok = false;
        int newAddr = queryCmd(QString("++addr\n")).trimmed().toInt(&ok);
        if(!ok || newAddr != a)
        {
            emit hardwareFailure();
            if(!ok)
                emit logMessage(QString("%1 address was not successfully set to %2. %1 failed to respond.").arg(d_prettyName).arg(a),LogHandler::Error);
            else
                emit logMessage(QString("%1 address was not successfully set to %2. Address is %3").arg(d_prettyName).arg(a).arg(newAddr),LogHandler::Error);
            return false;
        }
        d_currentAddress = a;

    }
	return true;
}
