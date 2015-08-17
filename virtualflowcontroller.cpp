#include "virtualflowcontroller.h"

#include "virtualinstrument.h"

VirtualFlowController::VirtualFlowController(QObject *parent) : FlowController(parent)
{
    d_subKey = QString("virtual");
    d_prettyName = QString("Virtual Flow Controller");

    p_comm = new VirtualInstrument(d_key,d_subKey,this);
}

VirtualFlowController::~VirtualFlowController()
{
}



bool VirtualFlowController::testConnection()
{
    p_readTimer->stop();
    readAll();
    p_readTimer->start();

    emit connected();
    return true;
}

void VirtualFlowController::initialize()
{
    FlowController::initialize();

    testConnection();
}

double VirtualFlowController::setFlowSetpoint(const int ch, const double val)
{
    if(ch<0 || ch >= d_config.size())
        return -1.0;

    d_config.set(ch,QtFTM::FlowSettingSetpoint,val);
    return readFlowSetpoint(ch);
}

double VirtualFlowController::setPressureSetpoint(const double val)
{
    d_config.setPressureSetpoint(val);
    return readPressureSetpoint();
}

double VirtualFlowController::readFlowSetpoint(const int ch)
{
    if(ch < 0 || ch >= d_config.size())
        return -1.0;

    emit flowSetpointUpdate(ch,d_config.setting(ch,QtFTM::FlowSettingSetpoint).toDouble());
    return d_config.setting(ch,QtFTM::FlowSettingSetpoint).toDouble();
}

double VirtualFlowController::readPressureSetpoint()
{
    emit pressureSetpointUpdate(d_config.pressureSetpoint());
    return d_config.pressureSetpoint();
}

double VirtualFlowController::readFlow(const int ch)
{
    if(ch < 0 || ch >= d_config.size())
        return -1.0;

    double sp = d_config.setting(ch,QtFTM::FlowSettingSetpoint).toDouble();
    double noise = sp*((double)(qrand()%100)-50.0)/1000.0;
    double flow = sp + noise;
    d_config.set(ch,QtFTM::FlowSettingFlow,flow);

    emit flowUpdate(ch,d_config.setting(ch,QtFTM::FlowSettingFlow).toDouble());
    return d_config.setting(ch,QtFTM::FlowSettingSetpoint).toDouble();
}

double VirtualFlowController::readPressure()
{
    d_config.setPressure(d_config.pressureSetpoint());

    emit pressureUpdate(d_config.pressure());
    return d_config.pressure();
}

void VirtualFlowController::setPressureControlMode(bool enabled)
{
    d_config.setPressureControlMode(enabled);
    readPressureControlMode();
}

bool VirtualFlowController::readPressureControlMode()
{
    emit pressureControlMode(d_config.pressureControlMode());
    return d_config.pressureControlMode();
}
