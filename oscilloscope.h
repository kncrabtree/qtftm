#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <QObject>
#include "fid.h"
#include "tcpinstrument.h"
#include <QStringList>
#include <QTime> // TESTING

class Oscilloscope : public TcpInstrument
{
	Q_OBJECT
public:
	explicit Oscilloscope(QObject *parent = nullptr);
	static Fid parseWaveform(const QByteArray d, double probeFreq);
    ~Oscilloscope();

    enum Resolution {
        Res_1kHz,
        Res_2kHz,
        Res_5kHz,
        Res_10kHz
    };

signals:
	void fidAcquired(const QByteArray d);
    void statusMessage(const QString s);
	
public slots:
	void initialize();
	bool testConnection();
    void configure();
	void replyReceived();
    void setResolution();
    void sendCurveQuery();
    void setActive(bool active = true) { d_acquisitionActive = active; }
    void wakeTheFUp();

private:
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

#endif // OSCILLOSCOPE_H
