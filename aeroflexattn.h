#ifndef AEROFLEXATTN_H
#define AEROFLEXATTN_H

#include "attenuator.h"

class AeroflexAttn : public Attenuator
{
    Q_OBJECT
public:
    explicit AeroflexAttn(QObject *parent = nullptr);


    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // Attenuator interface
public slots:
    int setAttn(int a);
    int readAttn();

private:
    double d_min;
    double d_max;

};

#endif // AEROFLEXATTN_H
