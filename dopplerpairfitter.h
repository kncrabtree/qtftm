#ifndef DOPPLERPAIRFITTER_H
#define DOPPLERPAIRFITTER_H

#include "abstractfitter.h"

class DopplerPairFitter : public AbstractFitter
{
	Q_OBJECT
public:
    DopplerPairFitter(QObject *parent = nullptr);

public slots:
	FitResult doFit(const Scan s);
};


#endif // DOPPLERPAIRFITTER_H
