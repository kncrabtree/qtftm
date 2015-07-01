#ifndef GPIBLANCONTROLLER_H
#define GPIBLANCONTROLLER_H

#include <QObject>
#include "datastructs.h"
#include <QtNetwork/QTcpSocket>
#include <QMutex>
#include <QMutexLocker>
#include "ftmsynthesizer.h"
#include "drsynthesizer.h"
#include "pulsegenerator.h"
#include "tcpinstrument.h"
#include <QTimer>

class GpibLanController : public TcpInstrument
{
	Q_OBJECT
public:
	explicit GpibLanController(QObject *parent = nullptr);

//	const FtmSynthesizer* ftSynth(){ return d_ftmSynth; }
//	const DrSynthesizer* drSynth(){ return d_drSynth; }
//	const FlowController* flowController(){ return d_mks; }
//	const PulseGenerator* pulseGenerator(){ return d_pGen; }
	
signals:
    void numInstruments(int);
	void ftmSynthFreq(double);
	void probeFreqUpdate(double);
    void rangeChanged();
    void ftmSynthBandChanged(int);
    void drSynthBandChanged(int);

#ifndef CONFIG_NODRSYNTH
	void drSynthFreq(double);
	void drSynthPower(double);
#endif

	void testComplete(QString,bool,QString);
	
public slots:
	void initialize();
	bool testConnection();
	void testObjectConnection(QString key);
	void sleep(bool b);

	void setFtmCavityFreqFromUI(double f);
    void ftmDeltaFreq(double delta);

#ifndef CONFIG_NODRSYNTH
	double setDrSynthFreq(double f);
	double setDrSynthPower(double p);
    void drSynthChangeBandFromUi();
#endif

    void ftmSynthChangeBandFromUi();


	double goToFtmProbeFreq();
    bool goToFtmCavityFreq();
    bool setFtmCavityFreq(double d);

private:
	QMutex mutex;
	int d_currentAddress;

	//GPIB instruments
	FtmSynthesizer *d_ftmSynth;

#ifndef CONFIG_NODRSYNTH
	DrSynthesizer *d_drSynth;
#endif

	QList<GpibInstrument*> d_instruments;
	bool setAddress(int a);

};

#endif // GPIBLANCONTROLLER_H
