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
    void receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d);
    void print();
    void toggleBatchLabels(bool on);
    QMenu *contextMenu();

    void replot();

private:
    bool d_hideBatchLabels;
    QList<QwtPlotMarker*> d_plotMarkers;
    double calculateAxisMaxWithLabel(Axis axis) const;
    QPair<double,double> calculateMarkerBoundaries(QFontMetrics fm, int index);

    // ZoomPanPlot interface
protected:
    void filterData();
};

#endif // BATCHSCANPLOT_H
