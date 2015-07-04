#ifndef VIRTUALATTENUATOR_H
#define VIRTUALATTENUATOR_H

#include "attenuator.h"

class VirtualAttenuator : public Attenuator
{
	Q_OBJECT
public:
	explicit VirtualAttenuator(QObject *parent = nullptr);
	~VirtualAttenuator();

	// HardwareObject interface
public slots:
	void initialize();
	bool testConnection();

	// Attenuator interface
public slots:
	int setAttn(int a);
	int readAttn();

private:
	int d_currentAttn;
};

#endif // VIRTUALATTENUATOR_H
