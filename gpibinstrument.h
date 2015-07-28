#ifndef GPIBINSTRUMENT_H
#define GPIBINSTRUMENT_H

#include "communicationprotocol.h"
#include "gpibcontroller.h"

class GpibInstrument : public CommunicationProtocol
{
    Q_OBJECT
public:
    explicit GpibInstrument(QString key, QString subKey, GpibController *c, QObject *parent = nullptr);
    virtual ~GpibInstrument();
    void setAddress(int a);
    int address() const;

    QIODevice *device() { return nullptr; }

protected:
    GpibController *p_controller;
    int d_address;

    // CommunicationProtocol interface
public:
    bool writeCmd(QString cmd);
    bool writeBinary(QByteArray dat);
    QByteArray queryCmd(QString cmd);

public slots:
    virtual void initialize();
    virtual bool testConnection();
};

#endif // GPIBINSTRUMENT_H
