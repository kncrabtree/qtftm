#include "pulseplot.h"
#include <QSettings>
#include <QApplication>

PulsePlot::PulsePlot(QWidget *parent) :
	QwtPlot(parent), numChannels(8)
{
	setTitle(QwtText("Pulses"));

	QFont labelFont(QString("sans serif"),8);
	QwtText xLabel(QString("Time (<span>&mu;</span>s)"));
	xLabel.setFont(labelFont);
	setAxisTitle(QwtPlot::xBottom, xLabel);
	enableAxis(QwtPlot::yLeft,false);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

	//each pulse gets 1.5 vertical units of space. channel 0 is on top, and 7 is on bottom.

	QPen p(QPalette().color(QPalette::Text));
	QPen dotP(p);
	dotP.setStyle(Qt::DotLine);
	for(int i=0; i<numChannels; i++)
	{
		pConfig.append(PulseGenerator::PulseChannelConfiguration(i+1,QString(""),i<3,0.0,0.0,PulseGenerator::ActiveHigh));

		double midpoint = (double)(numChannels - 1 - i)*1.5 + 0.75;
		double top = (double)(numChannels-i)*1.5;

		QwtPlotMarker *sep = new QwtPlotMarker;
		sep->setLineStyle(QwtPlotMarker::HLine);
		sep->setLinePen(dotP);
		sep->setYValue(top);
		sep->attach(this);

		QwtPlotCurve *c = new QwtPlotCurve;
		c->setPen(p);
		c->attach(this);
		c->setVisible(false);

		QwtPlotMarker *m = new QwtPlotMarker;
		QwtText text;
		text.setFont(labelFont);
		text.setColor(QPalette().color(QPalette::Text));
		m->setLabel(text);
		m->setLabelAlignment(Qt::AlignLeft);
		m->setValue(0.0, midpoint);
		m->attach(this);
		m->setVisible(false);

		plotItems.append(QPair<QwtPlotCurve*,QwtPlotMarker*>(c,m));

	}

}

void PulsePlot::pConfigAll(const QList<PulseGenerator::PulseChannelConfiguration> l)
{
	pConfig = l;
	replot();
}

void PulsePlot::pConfigSingle(const PulseGenerator::PulseChannelConfiguration c)
{
	int index = c.channel-1;
	pConfig[index] = c;
	replot();
}

void PulsePlot::pConfigSetting(const int ch, const PulseGenerator::Setting s, const QVariant val)
{
	if(ch-1>=pConfig.length())
		return;
	switch(s)
	{
	case PulseGenerator::Delay:
		pConfig[ch-1].delay = val.toDouble();
		break;
	case PulseGenerator::Width:
		pConfig[ch-1].width = val.toDouble();
		break;
	case PulseGenerator::Enabled:
		pConfig[ch-1].enabled = val.toBool();
		break;
	case PulseGenerator::Active:
		pConfig[ch-1].active = (PulseGenerator::ActiveLevel)val.toInt();
		break;
	case PulseGenerator::Name:
		pConfig[ch-1].channelName = val.toString();
		break;
	}
	replot();
}

void PulsePlot::replot()
{
	double maxTime = 0.0;
    for(int i=0; i< pConfig.size(); i++)
	{
		if(pConfig.at(i).enabled)
			maxTime = qMax(maxTime,pConfig.at(i).delay+pConfig.at(i).width);
	}

	maxTime*=1.25;

    for(int i=0; i<pConfig.size(); i++)
	{
		if(i>=pConfig.size() || i>=plotItems.size())
			break;

		PulseGenerator::PulseChannelConfiguration c = pConfig.at(i);
		double channelLow = (double)(numChannels-1-i)*1.5+0.25;
		double channelHigh = (double)(numChannels-1-i)*1.5+1.25;
		QVector<QPointF> data;
		if(c.active == PulseGenerator::ActiveLow)
		{
			data.append(QPointF(0.0,channelHigh));
			if(c.width>0.0 && c.enabled)
			{
				data.append(QPointF(c.delay,channelHigh));
				data.append(QPointF(c.delay,channelLow));
				data.append(QPointF(c.delay+c.width,channelLow));
				data.append(QPointF(c.delay+c.width,channelHigh));
			}
			data.append(QPointF(maxTime,channelHigh));
		}
		else
		{
			data.append(QPointF(0.0,channelLow));
			if(c.width>0.0 && c.enabled)
			{
				data.append(QPointF(c.delay,channelLow));
				data.append(QPointF(c.delay,channelHigh));
				data.append(QPointF(c.delay+c.width,channelHigh));
				data.append(QPointF(c.delay+c.width,channelLow));
			}
			data.append(QPointF(maxTime,channelLow));
		}
		plotItems[i].first->setSamples(data);
		if(!plotItems.at(i).first->isVisible())
			plotItems[i].first->setVisible(true);
		if(c.channelName != plotItems.at(i).second->label().text())
		{
			QwtText t = plotItems[i].second->label();
			t.setText(c.channelName);
			plotItems[i].second->setLabel(t);
		}
		plotItems[i].second->setXValue(maxTime);
		if(!plotItems.at(i).second->isVisible())
			plotItems[i].second->setVisible(true);
	}

	QwtPlot::replot();
}
