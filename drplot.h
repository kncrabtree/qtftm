#ifndef DRPLOT_H
#define DRPLOT_H

#include "abstractbatchplot.h"


class DrPlot : public AbstractBatchPlot
{
    Q_OBJECT
public:
    explicit DrPlot(int num, QWidget *parent = nullptr);

    // AbstractBatchPlot interface
public slots:
    void receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d);
    void print();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *ev);

};

#endif // DRPLOT_H
