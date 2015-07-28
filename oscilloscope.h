#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include "hardwareobject.h"
#include "fid.h"



class Oscilloscope : public HardwareObject
{
	Q_OBJECT
public:
	explicit Oscilloscope(QObject *parent = nullptr);	
    virtual ~Oscilloscope();

signals:
	void fidAcquired(const QByteArray d);
    void statusMessage(const QString s);
	
public slots:
    virtual void setResolution() =0;
    virtual void sendCurveQuery() =0;
    virtual void setActive(bool active = true) { d_acquisitionActive = active; }

private:
    bool d_acquisitionActive;
};

#ifdef QTFTM_OSCILLOSCOPE
#if QTFTM_OSCILLOSCOPE == 1
#include "dpo3012.h"
class DPO3012;
typedef DPO3012 OscilloscopeHardware;
#else
#include "virtualscope.h"
class VirtualScope;
typedef VirtualScope OscilloscopeHardware;
#endif
#endif

#endif // OSCILLOSCOPE_H
