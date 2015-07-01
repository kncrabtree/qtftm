#include "nofitter.h"

NoFitter::NoFitter(QObject *parent) : AbstractFitter(FitResult::NoFitting, parent)
{
	d_bufferGas = Analysis::bufferNe;
	d_temperature = 293.15;
}

FitResult NoFitter::doFit(const Scan s)
{
	Q_UNUSED(s)
	emit fitComplete(FitResult());
	return FitResult();
}
