#include "tcpinstrument.h"
#include <QTime>

TcpInstrument::TcpInstrument(QString key, QString subKey, QObject *parent) :
    CommunicationProtocol(CommunicationProtocol::Tcp,key,subKey,parent)
{
}

TcpInstrument::~TcpInstrument()
{
    disconnectSocket();
}

void TcpInstrument::initialize()
{
    p_socket = new QTcpSocket(this);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QString ip = s.value(key().append(QString("/ip")),QString("")).toString();
	int port = s.value(key().append(QString("/port")),5000).toInt();

	setSocketConnectionInfo(ip,port);
}

bool TcpInstrument::testConnection()
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QString ip = s.value(key().append(QString("/ip")),QString("")).toString();
	int port = s.value(key().append(QString("/port")),5000).toInt();

    setSocketConnectionInfo(ip,port);
	return connectSocket();

}

void TcpInstrument::socketError(QAbstractSocket::SocketError)
{
	//consider handling errors here at the socket level
}

bool TcpInstrument::writeCmd(QString cmd)
{
    if(p_socket->state() != QTcpSocket::ConnectedState)
    {
        if(!connectSocket())
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not write command. Socket is not connected. (Command = %1)").arg(cmd));
            return false;
        }
    }

    return writeBinary(cmd.toLatin1());
}

bool TcpInstrument::writeBinary(QByteArray dat)
{
    if(p_socket->state() != QTcpSocket::ConnectedState)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not write data. Socket is not connected. (Data = %1)").arg(QString(dat)),QtFTM::LogError);
        return false;
    }

    qint64 ret = p_socket->write(dat);

    if(ret == -1)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not write data. Write failed. (Data = %1, Hex = %2)")
                        .arg(QString(dat).arg(QString(dat.toHex()))),QtFTM::LogError);
        return false;
    }

    while (p_socket->bytesToWrite() > 0) {
        if(!p_socket->waitForBytesWritten(30000))
        {
            emit hardwareFailure();
            emit logMessage(QString("Timed out while waiting for write to finish. Data = %1")
                            .arg(QString(dat)),QtFTM::LogError);
            return false;
        }
    }
    return true;
}

QByteArray TcpInstrument::queryCmd(QString cmd)
{
    if(p_socket->state() != QTcpSocket::ConnectedState)
    {
        if(!connectSocket())
        {
            emit hardwareFailure();
            emit logMessage(QString("Could not write query. Socket is not connected. (Query = %1)").arg(cmd));
            return QByteArray();
        }
    }

    if(p_socket->bytesAvailable())
        p_socket->readAll();

    if(!writeCmd(cmd))
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not write query to %1. (query = %2)").arg(d_prettyName).arg(cmd),QtFTM::LogError);
        return QByteArray();
    }

	//write to socket here, return response
    if(!d_useTermChar || d_readTerminator.isEmpty())
    {
        if(!p_socket->waitForReadyRead(d_timeOut))
        {
            emit hardwareFailure();
            emit logMessage(QString("Did not respond to query. (query = %1)").arg(cmd),QtFTM::LogError);
            return QByteArray();
        }

        return p_socket->readAll();
    }
    else
    {
        QByteArray out;
        bool done = false;
        while(!done)
        {
            if(!p_socket->waitForReadyRead(d_timeOut))
                break;

            out.append(p_socket->readAll());
            if(out.endsWith(d_readTerminator))
                return out;
        }

        emit hardwareFailure();
        emit logMessage(QString("Timed out while waiting for termination character. (query = %1, partial response = %2)").arg(cmd).arg(QString(out)),QtFTM::LogError);
        emit logMessage(QString("Hex response: %1").arg(QString(out.toHex())));
        return out;
    }
    return QByteArray();
}

bool TcpInstrument::connectSocket()
{
    if(p_socket->state() == QAbstractSocket::ConnectedState)
    {
        p_socket->disconnectFromHost();
        if((p_socket->state() != QAbstractSocket::UnconnectedState) && !p_socket->waitForDisconnected(d_timeOut))
        {
            emit logMessage(QString("Timed out while waiting for disconnection."));
            return false;
        }
    }

    p_socket->connectToHost(d_ip,d_port);
    if(!p_socket->waitForConnected(d_timeOut))
    {
        emit logMessage(QString("Could not connect to %1:%2. %3").arg(d_ip).arg(d_port).arg(p_socket->errorString()),QtFTM::LogError);
        return false;
    }
    p_socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    p_socket->setSocketOption(QAbstractSocket::LowDelayOption,1);
    return true;
}

bool TcpInstrument::disconnectSocket()
{
    p_socket->disconnectFromHost();
    if(p_socket->state() == QAbstractSocket::ConnectedState)
        return p_socket->waitForDisconnected(d_timeOut);

    return true;
}

void TcpInstrument::setSocketConnectionInfo(QString ip, int port)
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.setValue(key().append(QString("/ip")),ip);
	s.setValue(key().append(QString("/port")),port);
	s.sync();

	d_ip = ip;
	d_port = port;
}
