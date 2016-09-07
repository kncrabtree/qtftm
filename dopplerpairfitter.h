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

private:
    double estimateSplitting(const FitResult::BufferGas &bg, double stagT, double frequency);
    QList<FitResult::DopplerPairParameters> estimateDopplerCenters(QList<QPair<QPointF,double> > peakList, double splitting, double ftSpacing, double tol);
    static bool dpAmplitudeLess(const FitResult::DopplerPairParameters &left, const FitResult::DopplerPairParameters &right);
};


#endif // DOPPLERPAIRFITTER_H
