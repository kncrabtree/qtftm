#ifndef GPIBINSTRUMENT_H
#define GPIBINSTRUMENT_H

#include <QTcpSocket>
#include "hardwareobject.h"


class GpibInstrument : public HardwareObject
{
	Q_OBJECT
public:
	explicit GpibInstrument(QString key, QString name, QTcpSocket *s, QObject *parent);

	void setAddress(const int a);
	int address() const { return d_address; }
	
signals:

public slots:
	void initialize();
    bool testConnection();

protected:
	QTcpSocket *d_socket;
	int d_address;

	bool writeCmd(QString cmd);
	QByteArray queryCmd(QString cmd);


	
};

#endif // GPIBINSTRUMENT_H
