#include "flowconfig.h"

FlowConfig::FlowConfig() : data(new FlowConfigData)
{
}

FlowConfig::FlowConfig(const FlowConfig &rhs) : data(rhs.data)
{

}

FlowConfig &FlowConfig::operator=(const FlowConfig &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

FlowConfig::~FlowConfig()
{

}

QVariant FlowConfig::setting(int index, QtFTM::FlowSetting s) const
{
    QVariant out;
    if(index < 0 || index > data->configList.size())
        return out;

    switch(s) {
    case QtFTM::FlowSettingEnabled:
        out = data->configList.at(index).enabled;
        break;
    case QtFTM::FlowSettingSetpoint:
        out = data->configList.at(index).setpoint;
        break;
    case QtFTM::FlowSettingFlow:
        out = data->flowList.at(index);
        break;
    case QtFTM::FlowSettingName:
        out = data->configList.at(index).name;
        break;
    }

    return out;
}


double FlowConfig::pressureSetpoint() const
{
    return data->pressureSetpoint;
}

double FlowConfig::pressure() const
{
    return data->pressure;
}

bool FlowConfig::pressureControlMode() const
{
    return data->pressureControlMode;
}

int FlowConfig::size() const
{
    return data->configList.size();
}

void FlowConfig::add(double set, QString name)
{
    QtFTM::FlowChannelConfig cc;
    cc.enabled = !(qFuzzyCompare(1.0+set,1.0));
    cc.name = name;
    cc.setpoint = set;
    data->configList.append(cc);
    data->flowList.append(0.0);
}

void FlowConfig::set(int index, QtFTM::FlowSetting s, QVariant val)
{
    if(index < 0 || index >= data->configList.size())
        return;

    switch(s) {
    case QtFTM::FlowSettingEnabled:
        //this is handled automatically by the setpoint case
        break;
    case QtFTM::FlowSettingSetpoint:
        data->configList[index].setpoint = val.toDouble();
        if(qFuzzyCompare(1.0+data->configList.at(index).setpoint,1.0))
            data->configList[index].enabled = false;
        else
            data->configList[index].enabled = true;
        break;
    case QtFTM::FlowSettingFlow:
        data->flowList[index] = val.toDouble();
        break;
    case QtFTM::FlowSettingName:
        data->configList[index].name = val.toString();
        break;
    }
}

void FlowConfig::setPressure(double p)
{
    data->pressure = p;
}


void FlowConfig::setPressureSetpoint(double s)
{
    data->pressureSetpoint = s;
}

void FlowConfig::setPressureControlMode(bool en)
{
    data->pressureControlMode = en;
}

QString FlowConfig::headerString() const
{
    QString out;
    out.append(QString("#Pressure\t%1\tkTorr\n").arg(pressure()));
    for(int i=0;i<data->configList.size(); i++)
    {
        out.append(QString("#Gas %1 name\t%2\t\n").arg(i+1).arg(data->configList.at(i).name));
        out.append(QString("#Gas %1 flow\t%2\tsccm\n").arg(i+1).arg(setting(i,QtFTM::FlowSettingFlow).toDouble()));
    }

    return out;
}

