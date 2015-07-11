#ifndef DRCORRPLOT_H
#define DRCORRPLOT_H

#include "abstractbatchplot.h"

class DrCorrPlot : public AbstractBatchPlot
{
	Q_OBJECT
public:
	explicit DrCorrPlot(int num, QWidget *parent = nullptr);
	~DrCorrPlot();

	// AbstractBatchPlot interface
public slots:
	void receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d);
	void print();

private:
	QList<QwtPlotZoneItem*> d_matchZones;
};

#endif // DRCORRPLOT_H
