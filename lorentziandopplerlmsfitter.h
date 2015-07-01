#ifndef LORENTZIANDOPPLERLMSFITTER_H
#define LORENTZIANDOPPLERLMSFITTER_H

#include "abstractfitter.h"

class LorentzianDopplerLMSFitter : public AbstractFitter
{
	Q_OBJECT
public:
	LorentzianDopplerLMSFitter(QObject *parent = nullptr);

public slots:
	FitResult doFit(const Scan s);
};


#endif // LORENTZIANDOPPLERLMSFITTER_H
