#include "oscilloscope.h"
#include <QTimer>
#include <QFile>
#include <QStringList>
#include <math.h>

Oscilloscope::Oscilloscope(QObject *parent) :
    HardwareObject(parent), d_acquisitionActive(false)
{
    d_key = QString("scope");
}

Oscilloscope::~Oscilloscope()
{
}
