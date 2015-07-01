#ifndef BATCHDR_H
#define BATCHDR_H

#include "batchmanager.h"


/*!
 * \brief A BatchManager that scans the DR frequency and integrates the resulting FT, keeping the FTM frequency constant.
 *
 * The DR scan is based on a reference Scan object that controls all settings of the instrument except the DR frequency, and whether the DR pulse is enabled.
 * The DR frequency is calculated from the start and step frequencies, and is completed when the stop frequency is scanned.
 * If the user requests calibration data, then each DR frequency will be scanned twice, once with the DR pulse off, and once with it on, and the resulting integrals ratioed.
 * The ranges list in the constructor contains the regions that should be integrated.
 * In the report, lists of the DR and calibration scans are listed, along with XY arrays of the ratioed data, signal integrals, and calibration integrals vs frequency (if there is no calibration, then just the signal vs frequency is given.
 * DR scan reports are stored in /home/data/QtFTM/dr/x/y/z.txt, where z is the drScan number, y are the thousands digits, and x are the millions digits (though it's unlikely that will ever go above 0!)
 *
 */
class BatchDR : public BatchManager
{
	Q_OBJECT
public:
    explicit BatchDR(Scan ftScan, double start, double stop, double step, int numScansBetween, QList<QPair<double,double> > ranges, bool doCal, AbstractFitter *f = new NoFitter());
	explicit BatchDR(int num, AbstractFitter *ftr = new NoFitter());

	int numScans() const { return d_numScans; }
	QList<QPair<double,double> > integrationRanges() const { return d_integrationRanges; }
	
signals:
	
public slots:

protected:
	Scan prepareNextScan();
	bool isBatchComplete();
	void processScan(Scan s);
	void writeReport();

private:
	Scan d_template;
	double d_start, d_stop, d_step;
    int d_numScansBetween;
	QList<QPair<double,double> > d_integrationRanges;

	int d_numScans;
	int d_completedScans;
	bool d_hasCalibration;
	bool d_lastScanWasCal;
	QList<QVector<QPointF> > d_drData;
	QList<QVector<double> > d_cal;
	QList<QVector<double> > d_dr;
	QList<int> d_scanNumbers;
	
};

#endif // BATCHDR_H
