#include "batchsingle.h"

BatchSingle::BatchSingle(Scan s, AbstractFitter *ftr) :
	BatchManager(QtFTM::SingleScan,false,ftr), d_completed(false), d_scan(s)
{
	d_totalShots = s.targetShots();
}

Scan BatchSingle::prepareNextScan()
{
    return d_scan;
}

void BatchSingle::advanceBatch(const Scan s)
{
    Q_UNUSED(s)
    d_completed = true;
}


void BatchSingle::processScan(Scan s)
{
	FitResult res = d_fitter->doFit(s);
	Q_UNUSED(res)  
}

bool BatchSingle::isBatchComplete()
{
	return d_completed;
}

void BatchSingle::writeReport()
{
	//there is no report for a single scan
}
