#include "abstractfitter.h"

AbstractFitter::AbstractFitter(const FitResult::FitterType t, QObject *parent) :
	QObject(parent), d_type(t)
{
}

AbstractFitter::~AbstractFitter()
{
}

QPair<QVector<QPointF>, double> AbstractFitter::doStandardFT(const Fid fid)
{
	return ftw.doFT(fid);
}
