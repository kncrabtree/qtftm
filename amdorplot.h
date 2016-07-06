#ifndef AMDORPLOT_H
#define AMDORPLOT_H

#include "zoompanplot.h"

class QwtPlotCurve;

class AmdorPlot : public ZoomPanPlot
{
    Q_OBJECT
public:
    AmdorPlot(QWidget *parent = nullptr);

    void setPlotRange(double min, double max);

public slots:
    void updateData(QList<QVector<QPointF> > l);

    // ZoomPanPlot interface
protected:
    void filterData();

    QList<QwtPlotCurve*> d_curves;
};

#endif // AMDORPLOT_H
