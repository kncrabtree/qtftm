#ifndef BATCH_H
#define BATCH_H
#include "batchmanager.h"

/*!
 * \brief A BatchManager for generic batch acquisitions.
 *
 * A generic batch acquisition consists of a list of scans that need to be executed.
 * These can optionally be considered calibration scans.
 * The report consists of a list of scan numbers, maximum FT intensities, and a string with additional info (Ft frequency, attenuation, etc).
 * Batch reports are stored in /home/data/QtFTM/batch/x/y/z.txt, where z is the batch number, y are the thousands digits, and x are the millions digits (though it's unlikely that will ever go above 0!)
 */
class Batch : public BatchManager
{
	Q_OBJECT
public:
    explicit Batch(QList<QPair<Scan,bool> > l, AbstractFitter *ftr = new NoFitter());
    explicit Batch(int num);
	
signals:
	
public slots:

protected:
	Scan prepareNextScan();
	bool isBatchComplete();
	void processScan(Scan s);
	void writeReport();

private:
    QList<QPair<Scan,bool> > d_scanList;
    QVector<QPointF> d_theData;
    QVector<QPointF> d_calData;
    QList<QPair<int,QPair<double,QString> > > d_saveData;

    QList<bool> d_loadCalList;
    int d_loadingIndex;
	
};

#endif // BATCH_H
