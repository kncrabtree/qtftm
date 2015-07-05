#ifndef SURVEYPLOT_H
#define SURVEYPLOT_H

#include "abstractbatchplot.h"

class SurveyPlot : public AbstractBatchPlot
{
    Q_OBJECT
public:
    explicit SurveyPlot(int num, QWidget *parent = nullptr);

    // AbstractBatchPlot interface
public slots:
    void receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d);
    void print();

};

#endif // SURVEYPLOT_H
