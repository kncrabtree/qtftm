#ifndef MKS947_H
#define MKS947_H

#include "flowcontroller.h"

class Mks946 : public FlowController
{
	
    //PBC Removed Q_OBJECT
public:
    explicit Mks946(QObject *parent = nullptr);	//PBC made explicit

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // FlowController interface
public slots:
    double setFlowSetpoint(const int ch, const double val);
    double setPressureSetpoint(const double val);
    double readFlowSetpoint(const int ch);
    double readPressureSetpoint();
    double readFlow(const int ch);
    double readPressure();
    void setPressureControlMode(bool enabled);
    bool readPressureControlMode();
    void poll();	//PBC

private:	//PBC made private
    bool mksWrite(QString cmd);
    QByteArray mksQuery(QString cmd);

    int d_address;
    int d_pressureChannel;
    int d_channelOffset; //Some models may have a pressure sensor module on channels 1 and 2, so flow channels would start at 3. This should contain the offset needed to convert logical channels (e.g., 0-3 in BlackChirp) to actual channel number on the device (must be 1-6).
	int d_numChannels;
	
    // HardwareObject interface
public slots:
    void readSettings();
    void sleep(bool b);

private:
    int d_nextRead;
    QString d_errorString;	//PBC taken from the blackchirp hardware object header
};

#endif // MKS947_H
