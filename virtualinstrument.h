#ifndef VIRTUALINSTRUMENT_H
#define VIRTUALINSTRUMENT_H

#include "communicationprotocol.h"

class VirtualInstrument : public CommunicationProtocol
{
public:
	explicit VirtualInstrument(QString key, QString subKey, QObject *parent = nullptr);
	~VirtualInstrument();

	// CommunicationProtocol interface
public:
	bool writeCmd(QString cmd);
	bool writeBinary(QByteArray dat);
	QByteArray queryCmd(QString cmd);
	QIODevice *device();

public slots:
	void initialize();
	bool testConnection();
};

#endif // VIRTUALINSTRUMENT_H
