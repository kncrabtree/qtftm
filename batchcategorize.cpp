#include "batchcategorize.h"

BatchCategorize::BatchCategorize(QList<QPair<Scan, bool> > scanList, QList<CategoryTest> testList, AbstractFitter *ftr) :
	BatchManager(QtFTM::Categorize,false,ftr), d_scanList(scanList), d_testList(testList)
{

}

BatchCategorize::~BatchCategorize()
{

}



void BatchCategorize::writeReport()
{
}

void BatchCategorize::advanceBatch(const Scan s)
{
}

void BatchCategorize::processScan(Scan s)
{
}

Scan BatchCategorize::prepareNextScan()
{
}

bool BatchCategorize::isBatchComplete()
{
}
