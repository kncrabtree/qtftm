#ifndef DPO3012_H
#define DPO3012_H

#include "oscilloscope.h"

#include <QTime>
#include <QByteArray>

class QTcpSocket;

class DPO3012 : public Oscilloscope
{
    Q_OBJECT
public:
    explicit DPO3012(QObject *parent = nullptr);

public slots:
    void initialize();
    bool testConnection();
    void configure();
    void replyReceived();
    void setResolution();
    void sendCurveQuery();
    void wakeTheFUp();

private:
    QTcpSocket *p_socket;

    QByteArray d_waveformPrefix;
    QByteArray d_response;
    bool d_headerHashFound;
    int d_binaryDataLength;
    bool d_resolutionChangePending;

    QTime d_responseTimeout;
    bool d_acquisitionActive;
    bool d_waitingForReply;
    bool d_waitingForWakeUp;
};

#endif // DPO3012_H
