#include "pulsegenconfig.h"



PulseGenConfig::PulseGenConfig() : data(new PulseGenConfigData)
{

}

PulseGenConfig::PulseGenConfig(const PulseGenConfig &rhs) : data(rhs.data)
{

}

PulseGenConfig &PulseGenConfig::operator=(const PulseGenConfig &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

PulseGenConfig::~PulseGenConfig()
{

}

QtFTM::PulseChannelConfig PulseGenConfig::at(const int i) const
{
    Q_ASSERT(i>=0 && i < data->config.size());
    return data->config.at(i);
}

int PulseGenConfig::size() const
{
    return data->config.size();
}

bool PulseGenConfig::isEmpty() const
{
    return data->config.isEmpty();
}

QVariant PulseGenConfig::setting(const int index, const QtFTM::PulseSetting s) const
{
    if(index < 0 || index >= data->config.size())
        return QVariant();

    switch(s)
    {
    case QtFTM::PulseDelay:
        return data->config.at(index).delay;
        break;
    case QtFTM::PulseWidth:
        return data->config.at(index).width;
        break;
    case QtFTM::PulseEnabled:
        return data->config.at(index).enabled;
        break;
    case QtFTM::PulseLevel:
        return data->config.at(index).level;
        break;
    case QtFTM::PulseName:
        return data->config.at(index).channelName;
        break;
    default:
        break;
    }

    return QVariant();
}

QtFTM::PulseChannelConfig PulseGenConfig::settings(const int index) const
{
    if(index < 0 || index >= data->config.size())
        return QtFTM::PulseChannelConfig();

    return data->config.at(index);

}

double PulseGenConfig::repRate() const
{
    return data->repRate;
}

QString PulseGenConfig::headerString() const
{
    QString out;

    out.append(QString("#Rep rate\t%1\tHz\n").arg(data->repRate));
    for(int i=0; i<data->config.size(); i++)
    {
        QString prefix = QString("#Pulse ch %1 ").arg(i+1);
        out.append(prefix+QString("name\t%1\t\n").arg(data->config.at(i).channelName));
        out.append(prefix+QString("active level\t"));
        if(data->config.at(i).level == QtFTM::PulseLevelActiveHigh)
            out.append(QString("Active High\t\n"));
        else
            out.append(QString("Active Low\t\n"));
        out.append(prefix + QString("enabled\t%1\t\n").arg(data->config.at(i).enabled));
        out.append(prefix + QString("delay\t%1\tus\n").arg(data->config.at(i).delay));
        out.append(prefix + QString("width\t%1\tus\n").arg(data->config.at(i).width));
    }

    return out;
}

void PulseGenConfig::set(const int index, const QtFTM::PulseSetting s, const QVariant val)
{
    if(index < 0 || index >= data->config.size())
        return;

    switch(s)
    {
    case QtFTM::PulseDelay:
        data->config[index].delay = val.toDouble();
        break;
    case QtFTM::PulseWidth:
        data->config[index].width = val.toDouble();
        break;
    case QtFTM::PulseEnabled:
        data->config[index].enabled = val.toBool();
        break;
    case QtFTM::PulseLevel:
        data->config[index].level = static_cast<QtFTM::PulseActiveLevel>(val.toInt());
        break;
    case QtFTM::PulseName:
        data->config[index].channelName = val.toString();
        break;
    default:
        break;
    }
}

void PulseGenConfig::set(const int index, const QtFTM::PulseChannelConfig cc)
{
    if(index < 0 || index >= data->config.size())
        return;

    set(index,QtFTM::PulseDelay,cc.delay);
    set(index,QtFTM::PulseWidth,cc.width);
    set(index,QtFTM::PulseEnabled,cc.enabled);
    set(index,QtFTM::PulseLevel,cc.level);
    set(index,QtFTM::PulseName,cc.channelName);
}

void PulseGenConfig::add(const QString name, const bool enabled, const double delay, const double width, const QtFTM::PulseActiveLevel level)
{
    QtFTM::PulseChannelConfig cc;
    cc.channel = data->config.size()+1;
    cc.channelName = name;
    cc.enabled = enabled;
    cc.delay = delay;
    cc.width = width;
    cc.level = level;

    data->config.append(cc);
}

void PulseGenConfig::setRepRate(const double r)
{
    data->repRate = r;
}

void PulseGenConfig::setDrEnabled(bool en)
{
    set(QTFTM_PGEN_DRCHANNEL,QtFTM::PulseEnabled,en);
}

