#include "virtualpulsegenerator.h"

#include "virtualinstrument.h"

VirtualPulseGenerator::VirtualPulseGenerator(QObject *parent) : PulseGenerator(parent)
{
    d_subKey = QString("virtual");
    d_prettyName = QString("Virtual Pulse Generator");

    p_comm = new VirtualInstrument(d_key,d_subKey,this);
}

VirtualPulseGenerator::~VirtualPulseGenerator()
{

}



bool VirtualPulseGenerator::testConnection()
{
    blockSignals(true);
    readAll();
    blockSignals(false);

    emit configUpdate(d_config);
    emit connected();
    return true;
}

void VirtualPulseGenerator::initialize()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);

    s.beginReadArray(QString("channels"));
    for(int i=0; i<QTFTM_PGEN_NUMCHANNELS; i++)
    {
        s.setArrayIndex(i);
        QString name = s.value(QString("name"),QString("Ch%1").arg(i)).toString();
        double d = s.value(QString("defaultDelay"),0.0).toDouble();
        double w = s.value(QString("defaultWidth"),0.050).toDouble();
	   QVariant lvl = s.value(QString("level"),QtFTM::PulseLevelActiveHigh);
        bool en = s.value(QString("defaultEnabled"),false).toBool();

	   if(lvl == QVariant(QtFTM::PulseLevelActiveHigh))
		  d_config.add(name,en,d,w,QtFTM::PulseLevelActiveHigh);
        else
		  d_config.add(name,en,d,w,QtFTM::PulseLevelActiveLow);
    }
    s.endArray();

    d_config.setRepRate(s.value(QString("repRate"),10.0).toDouble());
    s.endGroup();
    s.endGroup();

    testConnection();
}

QVariant VirtualPulseGenerator::read(const int index, const QtFTM::PulseSetting s)
{
    emit settingUpdate(index,s,d_config.setting(index,s));
    return d_config.setting(index,s);
}

double VirtualPulseGenerator::readRepRate()
{
    emit repRateUpdate(d_config.repRate());
    return d_config.repRate();
}

bool VirtualPulseGenerator::set(const int index, const QtFTM::PulseSetting s, const QVariant val)
{
    d_config.set(index,s,val);
    read(index,s);
    return true;
}

bool VirtualPulseGenerator::setRepRate(double d)
{
    d_config.setRepRate(d);
    readRepRate();
    return true;
}
