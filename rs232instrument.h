#ifndef RS232INSTRUMENT_H
#define RS232INSTRUMENT_H

#include "communicationprotocol.h"
#include <QtSerialPort/qserialport.h>
#include <QtSerialPort/qserialportinfo.h>

class Rs232Instrument : public CommunicationProtocol
{
    Q_OBJECT
public:
    explicit Rs232Instrument(QString key, QString subKey, QObject *parent = 0);
    virtual ~Rs232Instrument();
    bool writeCmd(QString cmd);
    bool writeBinary(QByteArray dat);
    QByteArray queryCmd(QString cmd);

    QIODevice *device() { return p_sp; }


public slots:
    virtual void initialize();
    virtual bool testConnection();

protected:
    QSerialPort *p_sp;

};

#endif // RS232INSTRUMENT_H
