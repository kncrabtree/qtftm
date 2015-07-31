#ifndef BATCHSINGLE_H
#define BATCHSINGLE_H

#include "batchmanager.h"

/*!
 * \brief A BatchManager that only executes a single scan.
 *
 * This class brings a single scan into the BatchManager framework, so that all acquisitions follow the same path through the code.
 * It's an extremely simple implementation of the BatchManager class that only does one scan, and does not generate a report.
 */
class BatchSingle : public BatchManager
{
	Q_OBJECT
public:
	explicit BatchSingle(Scan s, AbstractFitter *ftr = new NoFitter());
	
signals:
	
public slots:
	
protected:
	Scan prepareNextScan();
    void advanceBatch(const Scan s);
	void processScan(Scan s);
	bool isBatchComplete();
	void writeReport();

private:
    bool d_completed;
    Scan d_scan;
};

#endif // BATCHSINGLE_H
