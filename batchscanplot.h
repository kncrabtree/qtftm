#ifndef BATCHSCANPLOT_H
#define BATCHSCANPLOT_H

#include "abstractbatchplot.h"

#include <qwt6/qwt_plot_marker.h>


class BatchScanPlot : public AbstractBatchPlot
{
    Q_OBJECT
public:
    explicit BatchScanPlot(int num, QWidget *parent = nullptr);

    // AbstractBatchPlot interface
public slots:
    void receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d);
    void print();
};

#endif // BATCHSCANPLOT_H
