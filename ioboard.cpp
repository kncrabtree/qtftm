#include "ioboard.h"

IOBoard::IOBoard(QObject *parent) :
    HardwareObject(parent)
{
    d_key = QString("ioboard");

    p_readTimer = new QTimer(this);
    connect(p_readTimer,&QTimer::timeout,this,&IOBoard::checkForTrigger);
}

IOBoard::~IOBoard()
{

}
