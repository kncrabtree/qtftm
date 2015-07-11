#include "drcorrplot.h"

#include <qwt6/qwt_symbol.h>

DrCorrPlot::DrCorrPlot(int num, QWidget *parent) :
	AbstractBatchPlot(QString("drCorrPlot"),parent)
{
	QFont labelFont(QString("sans serif"),8);
	QwtText plotTitle(QString("DR Correlation %1").arg(num));
	plotTitle.setFont(labelFont);

	setTitle(plotTitle);

	QwtText xText(QString("Scan number"));
	setAxisTitle(QwtPlot::xBottom,xText);
}

DrCorrPlot::~DrCorrPlot()
{

}



void DrCorrPlot::receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
	if(d_metaDataList.isEmpty())
	{
	    //for a batch, there will only be one data curve
	    QwtPlotCurve *curve = new QwtPlotCurve(QString("Batch Data"));
	    curve->setPen(QPen(QPalette().color(QPalette::Text)));
	    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
	    d_plotCurves.append(curve);
	    curve->attach(this);

	    PlotCurveMetaData pcmd;
	    pcmd.curve = curve;
	    pcmd.visible = true;
	    pcmd.yMin = 0.0;
	    pcmd.yMax = d.first().at(0).y();
	    d_plotCurveMetaData.append(pcmd);

	    d_plotCurveData.append(QVector<QPointF>());

	    p_calCurve = new QwtPlotCurve(QString("Calibration"));
	    p_calCurve->setAxes(QwtPlot::xBottom,QwtPlot::yRight);
	    p_calCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

	    //color from settings...
	    QColor highlight = QPalette().color(QPalette::Highlight);
	    p_calCurve->setPen(QPen(highlight));
	    p_calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(highlight),QPen(highlight),QSize(5,5)));

	    p_calCurve->attach(this);

	    setAxisAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);
	    setAxisAutoScaleRange(QwtPlot::yLeft,0.0,0.0);
	    setAxisAutoScaleRange(QwtPlot::yRight,0.0,0.0);
	}

	QwtPlotMarker *m = new QwtPlotMarker();
	m->setLineStyle(QwtPlotMarker::VLine);
	QColor c(Qt::black);
	c.setAlpha(0);
	m->setLinePen(QPen(c));
	QwtText t(md.text);
	t.setFont(QFont(QString("sans-serif"),7));
	t.setColor(QPalette().color(QPalette::Text));
	m->setLabel(t);
	m->setLabelAlignment(Qt::AlignTop|Qt::AlignHCenter);
	m->setXValue((double)md.scanNum);
	if(!d_hidePlotLabels)
	    m->attach(this);
	d_plotMarkers.append(m);

	d_metaDataList.append(md);
	if(md.badTune)
	    addBadZone(md);

	if(!md.isCal)
	{
	    for(int i=d_plotCurveData.at(0).size(); i<d.at(0).size(); i++)
	    {
		   d_plotCurveMetaData[0].yMin = qMin(d_plotCurveMetaData.at(0).yMin,d.at(0).at(i).y());
		   d_plotCurveMetaData[0].yMax = qMax(d_plotCurveMetaData.at(0).yMax,d.at(0).at(i).y());
	    }

	    d_plotCurves[0]->setSamples(d.at(0));
	    d_plotCurveData = d;
	}
	else
	{
	    double max = 0.0;
	    for(int i=d_calCurveData.size(); i<d.at(0).size(); i++)
		   max = qMax(max,d.at(0).at(i).y());

	    d_calCurveData = d.at(0);
	    p_calCurve->setSamples(d.at(0));
	}

	expandAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);

	if(md.drMatch && !md.badTune)
	{
		QwtPlotZoneItem *zone = new QwtPlotZoneItem();
		zone->setOrientation(Qt::Vertical);
		QColor c(Qt::green);
		c.setAlpha(128);
		zone->setBrush(QBrush(c));
		zone->setZ(10.0);
		c.setAlpha(0);
		zone->setPen(QPen(c));
		setZoneWidth(zone,md);
		zone->attach(this);
		d_matchZones.append(zone);
	}

	if(d_showZonePending)
	{
	    d_showZonePending = false;
	    if(md.scanNum == d_zoneScanNum)
		   setSelectedZone(d_zoneScanNum);
	}

	replot();

 }

void DrCorrPlot::print()
{
}
