#ifndef CATEGORYPLOT_H
#define CATEGORYPLOT_H

#include "abstractbatchplot.h"

class CategoryPlot : public AbstractBatchPlot
{
	Q_OBJECT
public:
	explicit CategoryPlot(int num, QWidget *parent = nullptr);
	~CategoryPlot();

	struct CategoryPrintGraph {
		double xMin;
		double xMax;
		double yMax;
		int pageNum;
		int rectNum;
		int refScanNum;
		int firstCatScanNum;
	};

	// AbstractBatchPlot interface
public slots:
	void receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d);
	void print();

private:
	QList<QwtPlotZoneItem*> d_matchZones;
	QMap<int,double> d_refMaxes;
	void categoryPrint(QPrinter *pr, int firstScan, int lastScan, int graphsPerPage, QString title);
};

#endif // CATEGORYPLOT_H
