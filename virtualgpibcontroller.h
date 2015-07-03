#ifndef VIRTUALGPIBCONTROLLER_H
#define VIRTUALGPIBCONTROLLER_H

#include "gpibcontroller.h"

class VirtualGpibController : public GpibController
{
public:
	explicit VirtualGpibController(QObject *parent = nullptr);
	~VirtualGpibController();

	// HardwareObject interface
public slots:
	bool testConnection();
	void initialize();

	// GpibController interface
protected:
	bool readAddress();
	bool setAddress(int a);
};

#endif // VIRTUALGPIBCONTROLLER_H
