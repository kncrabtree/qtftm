#include "gpibinstrument.h"


GpibInstrument::GpibInstrument(QString key, QString name, QTcpSocket *s, QObject *parent) :
	HardwareObject(key,name,parent), d_socket(s)
{
}

void GpibInstrument::initialize()
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	setAddress(s.value(key().append(QString("/address")),0).toInt());
    s.setValue(key().append(QString("/prettyName")),name());
}

bool GpibInstrument::testConnection()
{
    d_socket->write("++read eoi\n");
    if(!d_socket->flush())
    {
        emit logMessage(QString("Could not flush output buffer for %1").arg(d_prettyName));
        return false;
    }
    d_socket->waitForReadyRead(60);
    d_socket->readAll();
    return true;
}

void GpibInstrument::setAddress(const int a)
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.setValue(key().append(QString("/address")),a);
	s.sync();

	d_address = a;
}

bool GpibInstrument::writeCmd(QString cmd)
{
    d_socket->write(cmd.toLatin1());
    if(!d_socket->waitForBytesWritten(1000))
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not write command to %1. (Command = %2)").arg(d_prettyName).arg(cmd),QtFTM::LogError);
        return false;
    }

    return true;

}

QByteArray GpibInstrument::queryCmd(QString cmd)
{
    int maxAttempts = 10;
    d_socket->readAll();
    for(int i=0; i<maxAttempts; i++)
    {
        d_socket->write(QString(cmd).append(QString("++read eoi\n")).toLatin1());
        if(!d_socket->flush())
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not write query to %1. (query = %2)").arg(d_prettyName).arg(cmd),QtFTM::LogError);
            return QByteArray();
        }

        if(d_socket->waitForReadyRead(60))
            return d_socket->readAll();
    }

    emit hardwareFailure();
    emit logMessage(QString("%1 did not respond to query. (query = %2)").arg(d_prettyName).arg(cmd),QtFTM::LogError);
    return QByteArray();
}
