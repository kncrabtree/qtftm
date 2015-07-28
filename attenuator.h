#ifndef ATTENUATOR_H
#define ATTENUATOR_H

#include "hardwareobject.h"
#include <QFile>

class Attenuator : public HardwareObject
{
	Q_OBJECT
public:
	explicit Attenuator(QObject *parent = nullptr);
    virtual ~Attenuator();
	
signals:
	void attnUpdate(int);
    void taattnUpdate(int);
    void attenFileParseSuccess(bool);

public slots:
    virtual void initialize();
    void changeAttenFile(QString fileName);

    int setTuningAttn(double freq);
    virtual int setAttn(int a) =0;
    virtual int readAttn() =0;
    void clearAttenData();

private:
    QList<QPair<double,int> > d_attenData;
    bool parseAttenFile(QString fileName);
	
};

#ifdef QTFTM_ATTENUATOR
#if QTFTM_ATTENUATOR == 1
#include "aeroflexattn.h"
class AeroflexAttn;
typedef AeroflexAttn AttenuatorHardware;
#else
#include "virtualattenuator.h"
class VirtualAttenuator;
typedef VirtualAttenuator AttenuatorHardware;
#endif
#endif

#endif // ATTENUATOR_H
