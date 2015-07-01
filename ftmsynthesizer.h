#ifndef FTMSYNTHESIZER_H
#define FTMSYNTHESIZER_H

#include "synthesizer.h"

class FtmSynthesizer : public Synthesizer
{
	Q_OBJECT
public:
	explicit FtmSynthesizer(QTcpSocket *s, QObject *parent = nullptr);
	
signals:
	void newFreq(double);
	void newProbeFreq(double);
	
public slots:
	bool testConnection();

	void setCavityFreqFromUI(double d);
    bool setCavityFreq(double d);
	double goToProbeFreq();
	bool goToCavityFreq();

    void goToCavityDeltaFreq(double delta);

private:
	double d_targetCavityFreq;
	bool setSynthFreq(double d);

	double d_mixerOffset;
	double d_probeOffset;

protected:
    double readFreq();


	
};

#endif // FTMSYNTHESIZER_H
