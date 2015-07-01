#ifndef DRSYNTHESIZER_H
#define DRSYNTHESIZER_H

#include "synthesizer.h"

class DrSynthesizer : public Synthesizer
{
	Q_OBJECT
public:
	explicit DrSynthesizer(QTcpSocket *s, QObject *parent = nullptr);
	
signals:
	void newFreq(double);
	void newPower(double);
	
public slots:
	bool testConnection();
	double setFreq(double f);	

	double setPower(double p);
	double readPower();

private:
    double d_freq;
    double d_power;

protected:
    double readFreq();
	
};

#endif // DRSYNTHESIZER_H
