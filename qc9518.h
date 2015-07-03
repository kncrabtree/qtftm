#ifndef QC9518_H
#define QC9518_H

#include "pulsegenerator.h"

class QC9518 : public PulseGenerator
{
public:
    explicit QC9518(QObject *parent = nullptr);
};

#endif // QC9518_H
