#ifndef NOFITTER_H
#define NOFITTER_H

#include "abstractfitter.h"

class NoFitter : public AbstractFitter
{
	Q_OBJECT
public:
	NoFitter(QObject *parent = nullptr);

public slots:
	FitResult doFit(const Scan s);
};

#endif // NOFITTER_H
