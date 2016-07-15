#ifndef DRCORRPLOT_H
#define DRCORRPLOT_H

#include "abstractbatchplot.h"

class DrCorrPlot : public AbstractBatchPlot
{
	Q_OBJECT
public:
    explicit DrCorrPlot(int num, QtFTM::BatchType t, QWidget *parent = nullptr);
	~DrCorrPlot();

	struct DrCorrPrintGraph {
		double xMin;
		double xMax;
		double yMax;
		int pageNum;
		int rectNum;
		int refScanNum;
		int lastDrScanNum;
	};

	// AbstractBatchPlot interface
public slots:
	void receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d);
	void print();

private:
	QList<QwtPlotZoneItem*> d_matchZones;
	QMap<int,double> d_refMaxes;
	void doCorrPrint(QPrinter *pr, int firstScan, int lastScan, int graphsPerPage, QString title);

};

#endif // DRCORRPLOT_H
