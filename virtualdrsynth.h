#ifndef VIRTUALDRSYNTH_H
#define VIRTUALDRSYNTH_H

#include "drsynthesizer.h"

class VirtualDrSynth : public DrSynthesizer
{
public:
	explicit VirtualDrSynth(QObject *parent = nullptr);
	~VirtualDrSynth();

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

#endif // VIRTUALDRSYNTH_H
