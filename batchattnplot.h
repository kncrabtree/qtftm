#ifndef BATCHATTNPLOT_H
#define BATCHATTNPLOT_H

#include "abstractbatchplot.h"

class BatchAttnPlot : public AbstractBatchPlot
{
    Q_OBJECT
public:
    explicit BatchAttnPlot(int num, QWidget *parent = nullptr);

    // AbstractBatchPlot interface
public slots:
    void receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d);
    void print();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *ev);
};

#endif // BATCHATTNPLOT_H
