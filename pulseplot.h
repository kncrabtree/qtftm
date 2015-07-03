#ifndef PULSEPLOT_H
#define PULSEPLOT_H

#include "zoompanplot.h"

#include <QList>

#include "pulsegenconfig.h"

class QwtPlotCurve;
class QwtPlotMarker;

class PulsePlot : public ZoomPanPlot
{
    Q_OBJECT
public:
    PulsePlot(QWidget *parent = 0);
    ~PulsePlot();

    PulseGenConfig config();

public slots:
    void newConfig(const PulseGenConfig c);
    void newSetting(int index, QtFTM::PulseSetting s, QVariant val);
    void newRepRate(double d);
    void newProtDelay(int d);
    void newScopeDelay(int d);

    // ZoomPanPlot interface
protected:
    void updateVerticalMarkers();
    void filterData();
    void replot();

private:
    PulseGenConfig d_config;
    QList<QPair<QwtPlotCurve*,QwtPlotMarker*>> d_plotItems;
    QwtPlotMarker *p_protMarker;
    QwtPlotMarker *p_scopeMarker;
    int d_protDelay;
    int d_scopeDelay;

};

#endif // PULSEPLOT_H
