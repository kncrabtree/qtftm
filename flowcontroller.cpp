#include "flowcontroller.h"

FlowController::FlowController(QObject *parent) :
    HardwareObject(parent), d_nextRead(QTFTM_FLOW_NUMCHANNELS)
{
	d_isCritical = false;
    d_key = QString("flowController");

    p_readTimer = new QTimer(this);
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    int interval = s.value(QString("pollIntervalMs"),333).toInt();
    s.endGroup();
    p_readTimer->setInterval(interval);
    connect(p_readTimer,&QTimer::timeout,this,&FlowController::readNext);
}

FlowController::~FlowController()
{

}

void FlowController::initialize()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    s.beginReadArray(QString("channels"));
    for(int i=0;i<QTFTM_FLOW_NUMCHANNELS;i++)
    {
        s.setArrayIndex(i);
        d_config.add(0.0,s.value(QString("name"),QString("")).toString());
    }
    s.endArray();
    s.endGroup();
}

void FlowController::setChannelName(const int ch, const QString name)
{
    if(ch < d_config.size())
	   d_config.set(ch,QtFTM::FlowSettingName,name);

    emit channelNameUpdate(ch,name);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    s.beginWriteArray(QString("channels"));
    s.setArrayIndex(ch);
    s.setValue(QString("name"),name);
    s.endArray();
    s.endGroup();
}

void FlowController::updateInterval()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    int interval = s.value(QString("pollIntervalMs"),333).toInt();
    s.endGroup();
    p_readTimer->setInterval(interval);
}

void FlowController::readNext()
{
    if(d_nextRead < 0 || d_nextRead >= d_config.size())
    {
        readPressure();
        d_nextRead = 0;
    }
    else
    {
        readFlow(d_nextRead);
        d_nextRead++;
    }
}

void FlowController::readAll()
{
    for(int i=0; i<d_config.size(); i++)
    {
	   emit channelNameUpdate(i,d_config.setting(i,QtFTM::FlowSettingName).toString());
        readFlow(i);
        readFlowSetpoint(i);
    }

    readPressureSetpoint();
    readPressure();
    readPressureControlMode();
}
