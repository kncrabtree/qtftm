#ifndef VIRTUALIOBOARD_H
#define VIRTUALIOBOARD_H

#include "ioboard.h"

class VirtualIOBoard : public IOBoard
{
	Q_OBJECT
public:
	explicit VirtualIOBoard(QObject *parent = nullptr);
	~VirtualIOBoard();

	// HardwareObject interface
public slots:
	bool testConnection();
	void initialize();

	// IOBoard interface
public slots:
	long setCwMode(bool cw);
	void ftmSynthBand(int band);
	long setMagnet(bool mag);
	void checkForTrigger();
};

#endif // VIRTUALIOBOARD_H
