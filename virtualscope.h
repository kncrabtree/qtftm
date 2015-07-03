#ifndef VIRTUALSCOPE_H
#define VIRTUALSCOPE_H

#include "oscilloscope.h"

class VirtualScope : public Oscilloscope
{
public:
	explicit VirtualScope(QObject *parent = nullptr);
	~VirtualScope();

	// HardwareObject interface
public slots:
	bool testConnection();
	void initialize();

	// Oscilloscope interface
public slots:
	void setResolution();
	void sendCurveQuery();

private:
	QVector<double> d_virtualData;
	QVector<double> d_currentVirtualData;
	QByteArray d_waveformPrefix;
};

#endif // VIRTUALSCOPE_H
