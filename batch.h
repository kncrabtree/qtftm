#ifndef BATCH_H
#define BATCH_H
#include "batchmanager.h"

/*!
 * \brief A BatchManager for generic batch acquisitions.
 *
 * A generic batch acquisition consists of a list of scans that need to be executed.
 * These can optionally be considered calibration scans.
 * The report consists of a list of scan numbers, maximum FT intensities, and a string with additional info (Ft frequency, attenuation, etc).
 * Batch reports are stored in savePath/batch/x/y/z.txt, where z is the batch number, y are the thousands digits, and x are the millions digits (though it's unlikely that will ever go above 0!)
 */
class Batch : public BatchManager
{
	Q_OBJECT
public:
    explicit Batch(QList<QPair<Scan,bool> > l, AbstractFitter *ftr = new NoFitter());
    explicit Batch(int num, AbstractFitter *ftr = new NoFitter());

    struct ScanResult {
        Scan scan;
        bool isCal;
        double ftMax;
        FitResult result;
    };
	
signals:
	
public slots:

protected:
	Scan prepareNextScan();
	bool isBatchComplete();
    void advanceBatch(const Scan s);
	void processScan(Scan s);
	void writeReport();

private:
    QList<QPair<Scan,bool> > d_scanList;
    QVector<QPointF> d_theData;
    QVector<QPointF> d_calData;
    QList<ScanResult> d_saveData;

    QList<bool> d_loadCalList;
    int d_loadingIndex;
    bool d_processScanIsCal;
	
};

#endif // BATCH_H
