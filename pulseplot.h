#ifndef PULSEPLOT_H
#define PULSEPLOT_H

#include <qwt6/qwt_plot.h>
#include <QList>
#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_plot_marker.h>
#include "pulsegenerator.h"

class PulsePlot : public QwtPlot
{
	Q_OBJECT
public:
	explicit PulsePlot(QWidget *parent = nullptr);
	
signals:
	
public slots:
	void pConfigAll(const QList<PulseGenerator::PulseChannelConfiguration> l);
	void pConfigSingle(const PulseGenerator::PulseChannelConfiguration c);
	void pConfigSetting(const int ch, const PulseGenerator::Setting s, const QVariant val);

	void replot();

private:
	QList<QPair<QwtPlotCurve*,QwtPlotMarker*> > plotItems;
	QList<PulseGenerator::PulseChannelConfiguration> pConfig;

	int numChannels;

	
};

#endif // PULSEPLOT_H
