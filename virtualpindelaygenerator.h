#ifndef VIRTUALPINDELAYGENERATOR_H
#define VIRTUALPINDELAYGENERATOR_H

#include "pinswitchdrivedelaygenerator.h"

class VirtualPinDelayGenerator : public PinSwitchDriveDelayGenerator
{
	Q_OBJECT
public:
	explicit VirtualPinDelayGenerator(QObject *parent = nullptr);
	~VirtualPinDelayGenerator();

	// HardwareObject interface
public slots:
	bool testConnection();
	void initialize();

	// PinSwitchDriveDelayGenerator interface
public slots:
	int setProtectionDelay(int delayInMicroseconds);
	int setScopeDelay(int delayInMicroscconds);
	void readAll();
};

#endif // VIRTUALPINDELAYGENERATOR_H
