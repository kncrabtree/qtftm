#include "virtualscope.h"

#include "virtualinstrument.h"

#include <QFile>

VirtualScope::VirtualScope(QObject *parent) :
	Oscilloscope(parent)
{
	d_subKey = QString("virtual");
	d_prettyName = QString("Virtual Oscilloscope");

	p_comm = new VirtualInstrument(d_key,d_subKey,this);
}

VirtualScope::~VirtualScope()
{

}



bool VirtualScope::testConnection()
{
	setResolution();
	emit connected();
	return true;
}

void VirtualScope::initialize()
{
	QFile f(QString(":/virtualdata.txt"));
	if(f.open(QIODevice::ReadOnly))
	{
		while(!f.atEnd())
			d_virtualData.append(f.readLine().trimmed().toDouble());

		f.close();
	}

	d_waveformPrefix = QByteArray("1;x;x;x;RI;LSB;x;x;x;x;5e-7;x;x;x;4.08e-3;0");
	testConnection();
}

void VirtualScope::setResolution()
{
	//get current resolution setting
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QtFTM::ScopeResolution r = (QtFTM::ScopeResolution)s.value(QString("%1/%2/resolution")
												    .arg(d_key).arg(d_subKey),(int)QtFTM::Res_5kHz).toInt();

	d_currentVirtualData.clear();
	switch(r)
	{
	case QtFTM::Res_1kHz:
		d_currentVirtualData.resize(2000);
		break;
	case QtFTM::Res_2kHz:
		d_currentVirtualData.resize(1000);
		break;
	case QtFTM::Res_5kHz:
		d_currentVirtualData.resize(400);
		break;
	case QtFTM::Res_10kHz:
		d_currentVirtualData.resize(200);
		break;
	}

	for(int i=0; i<d_currentVirtualData.size(); i++)
		d_currentVirtualData[i] = d_virtualData.at(i);
}

void VirtualScope::sendCurveQuery()
{
	QByteArray out = QByteArray("#");
	QByteArray numBytes = QByteArray::number(d_currentVirtualData.size());
	out.append(QByteArray::number(numBytes.size())).append(numBytes);

	for(int i=0; i<d_currentVirtualData.size(); i++)
	{
		double d = d_currentVirtualData.at(i);
		d += static_cast<double>((qrand() % 200000) - 100000)/1e6;
		qint8 dat = qBound(-128,static_cast<int>(d/4.08e-3),127);
		out.append(dat);
	}
	out.append(QChar('\n'));

	emit fidAcquired(d_waveformPrefix+out);
}
