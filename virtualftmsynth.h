#ifndef VIRTUALFTMSYNTH_H
#define VIRTUALFTMSYNTH_H

#include "ftmsynthesizer.h"

class VirtualFtmSynth : public FtmSynthesizer
{
	Q_OBJECT
public:
	explicit VirtualFtmSynth(QObject *parent = nullptr);
	~VirtualFtmSynth();

	// HardwareObject interface
public slots:
	bool testConnection();
	void initialize();

	// Synthesizer interface
protected:
	double setSynthFreq(double d);
	double readSynthFreq();
	double setSynthPower(double d);
	double readSynthPower();

private:
	double d_currentSynthFreq;
	double d_currentSynthPower;
};

#endif // VIRTUALFTMSYNTH_H
