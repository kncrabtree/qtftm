#ifndef QC9518_H
#define QC9518_H

#include "pulsegenerator.h"

class QC9518 : public PulseGenerator
{
public:
    explicit QC9518(QObject *parent = nullptr);

	// HardwareObject interface
public slots:
	bool testConnection();
	void initialize();
	void sleep(bool b);

	// PulseGenerator interface
public slots:
	QVariant read(const int index, const QtFTM::PulseSetting s);
	double readRepRate();
	bool set(const int index, const QtFTM::PulseSetting s, const QVariant val);
	bool setRepRate(double d);

private:
	bool pGenWriteCmd(QString cmd);
};

#endif // QC9518_H
