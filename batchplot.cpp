#include "batchplot.h"
#include <qwt6/qwt_symbol.h>
#include <qwt6/qwt_picker_machine.h>
#include <QMenu>
#include <qwt6/qwt_scale_map.h>
#include <qwt6/qwt_scale_widget.h>
#include <QSettings>
#include <QApplication>
#include <qwt6/qwt_legend.h>
#include <QEvent>
#include <QMouseEvent>
#include <QColorDialog>
#include <qwt6/qwt_plot_canvas.h>
#include <math.h>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QtPrintSupport/QPrintDialog>
#include <qwt6/qwt_plot_renderer.h>
#include <qwt6/qwt_plot_grid.h>
#include <qwt6/qwt_legend_label.h>
#include <QtPrintSupport/QPrinter>
#include <QLabel>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>

BatchPlot::BatchPlot(QWidget *parent) :
    QwtPlot(parent), calCurve(nullptr), showZonePending(false), zoneScanNum(0), zoneIsSurveyCal(false), d_replot(true),
    panning(false), autoScaleXActive(true), autoScaleYActive(true), d_hideBatchLabels(false)
{
	QFont labelFont(QString("sans serif"),8);
	setAxisFont(QwtPlot::xBottom,labelFont);
	setAxisFont(QwtPlot::yLeft,labelFont);
	setAxisFont(QwtPlot::yRight,labelFont);

	freqText = QwtText(QString("Frequency (MHz)"));
	freqText.setFont(labelFont);

	scanText = QwtText(QString("Scan Number"));
	scanText.setFont(labelFont);

	QwtPlotPicker *picker = new QwtPlotPicker(this->canvas());
	picker->setAxis(QwtPlot::xBottom,QwtPlot::yLeft);
	picker->setStateMachine(new QwtPickerClickPointMachine);
	picker->setMousePattern(QwtEventPattern::MouseSelect1,Qt::RightButton);
	picker->setTrackerMode(QwtPicker::AlwaysOn);
	picker->setTrackerPen(QPen(QPalette().color(QPalette::Text)));
	picker->setEnabled(true);
	connect(picker,static_cast<void (QwtPlotPicker::*)(const QPointF&)>(&QwtPlotPicker::selected),this,&BatchPlot::contextMenu);

	zone = new QwtPlotZoneItem();
	zone->setOrientation(Qt::Vertical);
	QColor zoneColor = QPalette().color(QPalette::AlternateBase);
	zoneColor.setAlpha(128);
	zone->setBrush(QBrush(zoneColor));
	zone->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
//	zone->setItemAttribute(QwtPlotItem::AutoScale,true);
	zone->setInterval(0.0,0.0);

	setAxisAutoScale(QwtPlot::xBottom,false);
	setAxisAutoScale(QwtPlot::yLeft,false);

	//when the x axis is rescaled, the zone needs to be recalculated in some cases.
//	connect(axisWidget(QwtPlot::xBottom),&QwtScaleWidget::scaleDivChanged,[=](){ recalcZone(true); });
    canvas()->installEventFilter(this);
}

BatchPlot::~BatchPlot()
{
    zone->detach();
    delete zone;

    //this will delete everything attached to the plot
    //the QLists of pointers do not call delete on the individual items, so this should be safe
    detachItems();
}

void BatchPlot::prepareForNewBatch(BatchManager::BatchType t)
{
	if(t == BatchManager::SingleScan)
		return;

	zone->detach();
	showZonePending = false;
	zoneIsSurveyCal = false;
	zoneScanNum = -1;
    d_hideBatchLabels = false;

	if(calCurve)
	{
		calCurve->detach();
		delete calCurve;
		calCurve = nullptr;
	}

	while(!plotCurves.isEmpty())
	{
		QwtPlotCurve *c = plotCurves.takeFirst();
		c->detach();
		delete c;
	}

	while(!plotMarkers.isEmpty())
	{
		QwtPlotMarker *p = plotMarkers.takeFirst();
		p->detach();
		delete p;
	}

	detachItems(QwtPlotItem::Rtti_PlotLegend);

	metaData.clear();
	plotCurveData.clear();
	calCurveData.clear();
	yMinMaxes.clear();
	insertLegend(nullptr);
	panning = false;
	autoScaleXActive = true;
	autoScaleYActive = true;
    setTitle(QString(""));

	replot();

	switch(t)
	{
	case BatchManager::SingleScan:
		break;
	case BatchManager::Batch:
		setAxisTitle(QwtPlot::xBottom,scanText);
		show();
		break;
	default:
		setAxisTitle(QwtPlot::xBottom,freqText);
		show();
		break;
	}
}

void BatchPlot::receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{

	//Different types of batch scans need to show different things...
	switch(md.type)
	{
	case BatchManager::SingleScan:
		//this should never happen, because BatchSingle never emits the batchData signal!
		break;
	case BatchManager::Survey:
		surveyDataReceived(md,d);
		break;
	case BatchManager::DrScan:
		drDataReceived(md,d);
		break;
	case BatchManager::Batch:
		batchDataReceived(md,d);
		break;
    case BatchManager::Attenuation:
        attnDataReceived(md,d);
        break;
	}

    //attenuation scans aren't in frequency order.
    if(md.type != BatchManager::Attenuation)
        metaData.append(md);
    else
    {
        if(metaData.isEmpty() || md.maxXVal > metaData.at(metaData.size()-1).maxXVal)
            metaData.append(md);
        else
            metaData.prepend(md);
    }

	//since the analysis widget typically updates before this function is called, update the zone if necessary
	if(showZonePending)
	{
		if(!(md.type == BatchManager::DrScan && md.isCal))
		{
			showZonePending = false;
			if(md.scanNum == zoneScanNum)
				showZone(md.scanNum);
		}
	}
	else
	{
		//usually we want to replot. There are a few exceptions, though
        if(!(md.type == BatchManager::DrScan && md.isCal) && d_replot)
			replot();
	}
}

void BatchPlot::contextMenu(QPointF pos)
{
	if(metaData.isEmpty())
		return;

	QMenu *menu = new QMenu();
	menu->setAttribute(Qt::WA_DeleteOnClose);

	bool hasCal = false;
	bool hasScan = false;

    if(metaData.at(0).type != BatchManager::Attenuation)
    {
        for(int i=0; i<metaData.size(); i++)
        {
            if(metaData.at(i).isCal)
                hasCal = true;
            else
                hasScan = true;

            if(hasCal && hasScan)
                break;
        }
    }


	QAction *viewScanAction = menu->addAction(QString("View nearest scan"));
	if(!hasScan)
		viewScanAction->setEnabled(false);
	connect(viewScanAction,&QAction::triggered,this,&BatchPlot::loadScan);

	QAction *viewCalAction = menu->addAction(QString("View nearest cal scan"));
	if(!hasCal || metaData.at(0).type == BatchManager::Batch)
		viewCalAction->setEnabled(false);
	connect(viewCalAction,&QAction::triggered,this,&BatchPlot::loadCalScan);

	QAction *autoscaleAction = menu->addAction(QString("Autoscale axes"));
	if(autoScaleXActive && autoScaleYActive)
        autoscaleAction->setEnabled(false);
	connect(autoscaleAction,&QAction::triggered,this,&BatchPlot::enableAutoScaling);

    if(metaData.at(0).type == BatchManager::Batch)
    {
        QAction *hideLabelsAction = menu->addAction(QString("Hide labels"));
        hideLabelsAction->setCheckable(true);
        hideLabelsAction->setChecked(d_hideBatchLabels);
	   connect(hideLabelsAction,&QAction::triggered,this,&BatchPlot::hideLabelsCallback);
    }

    QAction *exportAction = menu->addAction(QString("Export XY..."));
    if(metaData.size() == 0)
	    exportAction->setEnabled(false);
    connect(exportAction,&QAction::triggered,this,&BatchPlot::exportXY);

	double x = canvasMap(QwtPlot::xBottom).transform(pos.x());
	double y = canvasMap(QwtPlot::yLeft).transform(pos.y());
	lastClickPos = pos;

	menu->popup(canvas()->mapToGlobal(QPoint((int)x,(int)y)));

}

void BatchPlot::setBatchTitle(QString text)
{
    QwtText t(text);
    t.setFont(QFont(QString("sans-serif"),8));
    setTitle(t);
}

void BatchPlot::hideLabelsCallback(bool hide)
{
    d_hideBatchLabels = hide;

    for(int i=0;i<plotMarkers.size();i++)
    {
        if(hide)
            plotMarkers[i]->detach();
        else
            plotMarkers[i]->attach(this);
    }

    replot();

}

void BatchPlot::loadCalScan()
{
	//we need to find the nearest calibration scan to where the mouse click happened
	if(metaData.isEmpty())
		return;

	double x = lastClickPos.x();

	int firstCalIndex = 0, lastCalIndex=0;

	//find positions of first and last cal scan in metadata list
	for(int i=0; i<metaData.size(); i++)
	{
		if(metaData.at(i).isCal)
		{
			firstCalIndex = i;
			break;
		}
	}
	for(int i=metaData.size()-1; i>-1; i--)
	{
		if(metaData.at(i).isCal)
		{
			lastCalIndex = i;
			break;
		}
	}

	bool scanningDown = false;
	//if this is a survey or DR measurement, it's possible to scan down in frequency
	//it if there's only 1 cal scan so far, it doesn't matter (this is only used to order multiple scans)
	if(metaData.at(firstCalIndex).minXVal > metaData.at(lastCalIndex).minXVal)
		scanningDown = true;

	//make a list of cal scans in increasing frequency
	QList<BatchManager::BatchPlotMetaData> calScans;
	for(int i=firstCalIndex; i<=lastCalIndex; i++)
	{
		if(metaData.at(i).isCal)
		{
			if(scanningDown)
				calScans.prepend(metaData.at(i));
			else
				calScans.append(metaData.at(i));
		}
	}

	//locate scan number of nearest cal scan
	if(x<calScans.at(0).minXVal)
	{
		emit requestScan(calScans.at(0).scanNum);
		return;
	}
	else if(x>calScans.at(calScans.size()-1).minXVal)
	{
		emit requestScan(calScans.at(calScans.size()-1).scanNum);
		return;
	}
	for(int i=0; i<calScans.size()-1; i++)
	{
		if(fabs(x-calScans.at(i).minXVal) < fabs(x-calScans.at(i+1).minXVal))
		{
			emit requestScan(calScans.at(i).scanNum);
			return;
		}
	}

	emit requestScan(calScans.at(calScans.size()-1).scanNum);

}

void BatchPlot::loadScan()
{
	//we need to find the closest non-cal scan to where the mouse click was
	if(metaData.isEmpty())
		return;

	double x = lastClickPos.x();

	//if a generic batch, we just round the x position and request nearest scan
	if(metaData.at(0).type == BatchManager::Batch)
	{
		int num = (int)round(x);
		if(num < metaData.at(0).scanNum)
			num = metaData.at(0).scanNum;
		else if(num > metaData.at(metaData.size()-1).scanNum)
			num = metaData.at(metaData.size()-1).scanNum;

		emit requestScan(num);
		return;
	}

	int firstScanIndex = 0, lastScanIndex=0;

	//find positions of first and last non-cal scan in metadata list
	for(int i=0; i<metaData.size(); i++)
	{
		if(!metaData.at(i).isCal)
		{
			firstScanIndex = i;
			break;
		}
	}
	for(int i=metaData.size()-1; i>-1; i--)
	{
		if(!metaData.at(i).isCal)
		{
			lastScanIndex = i;
			break;
		}
	}

	bool scanningDown = false;
	//if this is a survey or DR measurement, it's possible to scan down in frequency
	//it if there's only 1 cal scan so far, it doesn't matter (this is only used to order multiple scans)
	if(metaData.at(firstScanIndex).minXVal > metaData.at(lastScanIndex).minXVal)
		scanningDown = true;

	//make a list of non-cal scans in increasing frequency
	QList<BatchManager::BatchPlotMetaData> scans;
	for(int i=firstScanIndex; i<=lastScanIndex; i++)
	{
		if(!metaData.at(i).isCal)
		{
			if(scanningDown)
				scans.prepend(metaData.at(i));
			else
				scans.append(metaData.at(i));
		}
	}

	//locate scan number of nearest non-cal scan
	if(x<scans.at(0).minXVal)
	{
		emit requestScan(scans.at(0).scanNum);
		return;
	}
	else if(x>scans.at(scans.size()-1).maxXVal)
	{
		emit requestScan(scans.at(scans.size()-1).scanNum);
		return;
	}
	for(int i=0; i<scans.size()-1; i++)
	{
		if(x >= scans.at(i).minXVal && x < scans.at(i+1).minXVal)
		{
			emit requestScan(scans.at(i).scanNum);
			return;
		}
	}

	//if we've made it here, then it must be the last scan in the list
	emit requestScan(scans.at(scans.size()-1).scanNum);
}

void BatchPlot::showZone(int scanNum, bool doReplot)
{
	zoneScanNum = scanNum;
	zoneIsSurveyCal = false;

	//try to find scanNum in metadata list
	//metadata list will always be in increasing scan number, and will have no gaps
	if(metaData.isEmpty())
	{
		if(zone->plot() == this)
		{
			zone->detach();
			replot();
		}

		showZonePending = true;

		return;
	}

    if(metaData.at(0).scanNum < 0 && metaData.at(metaData.size()-1).scanNum < 0) //no scans associated with this batch (e.g. attn table)
    {
        showZonePending = false;
        return;
    }

	if(scanNum < metaData.at(0).scanNum || scanNum > metaData.at(metaData.size()-1).scanNum)
	{
		if(zone->plot() == this)
		{
			zone->detach();
			replot();
		}

		//when a scan is complete, the analysis widget gets updated BEFORE the batch manager sends out the new data
		//that means this function gets called before the scan has been put into the metadata array
		//so, if the scan number is 1 greater than the most recent entry, we'll want to draw the zone as soon as the data come in
		if(scanNum >= metaData.at(0).scanNum + metaData.size())
			showZonePending = true;

		return;
	}

	//if we've made it this far, the scan is in the list.
	int scanIndex = scanNum - metaData.at(0).scanNum;
	BatchManager::BatchPlotMetaData md = metaData.at(scanIndex);

	//set the zone interval. this is generally given by the metadata min and max
	//the exception: in a survey, a calibration scan is represented by a point, so the zone should be the marker width instead
	if(md.type == BatchManager::Survey && md.isCal)
	{
		//set a flag so that the width will be recalculated if the x scale changes
		zoneIsSurveyCal = true;


		double markHalfWidth = calCurve->symbol()->size().width()/2.0;
		QwtScaleMap map = canvasMap(QwtPlot::xBottom);

		//to calculate the width, we have to transform the marker position into its pixel position,
		//add the width of the marker, and transform it back to plot coordinates
		//this isn't perfect, because of rounding pixel positions, so an extra pixel is added on each side
		//to make sure the whole marker gets enclosed
		double zoneMin = map.invTransform(map.transform(md.minXVal)-markHalfWidth-1.0);
		double zoneMax = map.invTransform(map.transform(md.minXVal)+markHalfWidth+1.0);

		zone->setInterval(zoneMin,zoneMax);

	}
	else
	{
		zone->setInterval(md.minXVal,md.maxXVal);
	}

	//style is different for cal scans
	if(md.isCal)
		zone->setPen(QPen(QColor(QPalette().color(QPalette::Text)),1.0,Qt::DotLine));
	else
		zone->setPen(QPen(QColor(QPalette().color(QPalette::Text))));

	if(zone->plot() != this)
		zone->attach(this);

	if(doReplot)
		replot();

}

void BatchPlot::recalcZone(bool doReplot)
{
	//since in a survey, the width of the zone for a calibration scan depends on the marker width in pixels,
	//it needs to be recalculated whenever the x scale changes.
	if(zoneIsSurveyCal)
		showZone(zoneScanNum, doReplot);
}

void BatchPlot::filterData()
{
	if(plotCurveData.size() == 0)
		return;

	//the calibration curve should not be filtered (in survey mode). only the data curves need to be filtered
	//calculate pixel positions corresponding to scale minimum and maximum

//	if(autoScaleXActive)
		updateAxes();
		//updateAxes() doesn't reprocess the scale labels, which might need to be recalculated.
        //this line will take care of that and avoid plot glitches
        QApplication::sendPostedEvents(this,QEvent::LayoutRequest);

	QwtScaleMap map = canvasMap(QwtPlot::xBottom);
//	QwtScaleDiv div = axisScaleDiv(QwtPlot::xBottom);


	double firstPixel = 0.0;
	double lastPixel = canvas()->width();
	double scaleMin = map.invTransform(firstPixel);
	double scaleMax = map.invTransform(lastPixel);

	//for batch mode, filter labels if they're going to overlap
    if(metaData.at(0).type == BatchManager::Batch && !d_hideBatchLabels)
	{
		//loop over labels, seeing if the next will overlap
		int activeIndex = 0;
		double scaling = (scaleMax-scaleMin)/(double)canvas()->width();
		QFontMetrics fm(plotMarkers.at(0)->label().font());
		plotMarkers[0]->setVisible(true);
		for(int i=1; i<plotMarkers.size()-1; i++)
		{
			//calculate right edge of active marker, and left edge of current marker
			//get widest text line
			QStringList lines = plotMarkers.at(activeIndex)->label().text().split(QString("\n"),QString::SkipEmptyParts);
			int w = 0;
			for(int j=0; j<lines.size(); j++)
				w = qMax(w,fm.boundingRect(lines.at(j)).width());

			double activeRight = (double)metaData.at(activeIndex).scanNum + (double)w*scaling/2.0;

			lines = plotMarkers.at(i)->label().text().split(QString("\n"),QString::SkipEmptyParts);
			w = 0;
			for(int j=0; j<lines.size(); j++)
				w = qMax(w,fm.boundingRect(lines.at(j)).width());

			double activeLeft = (double)metaData.at(i).scanNum - (double)w*scaling/2.0;

			//hide this label if it overlaps with the active label
			if(activeLeft < activeRight)
				plotMarkers[i]->setVisible(false);
			else //otherwise, it is visible, and will be the reference for the next iteration of the loop
			{
				plotMarkers[i]->setVisible(true);
				activeIndex = i;
			}

		}

		plotMarkers[plotMarkers.size()-1]->setVisible(true);
		QStringList lines = plotMarkers.at(plotMarkers.size()-1)->label().text().split(QString("\n"),QString::SkipEmptyParts);
		int w = 0;
		for(int j=0; j<lines.size(); j++)
			w = qMax(w,fm.boundingRect(lines.at(j)).width());

		double finalLeft = (double)metaData.at(plotMarkers.size()-1).scanNum - (double)w*scaling/2.0;

		//we want the final label to be visible, so we need to hide the next-to-last visible label if it overlaps
		for(int i=plotMarkers.size()-1; i>2; i--)
		{
			if(!plotMarkers.at(i-1)->isVisible())
				continue;

			w = 0;
			lines = plotMarkers.at(i-1)->label().text().split(QString("\n"),QString::SkipEmptyParts);
			for(int j=0; j<lines.size(); j++)
				w = qMax(w,fm.boundingRect(lines.at(j)).width());
			double activeRight = (double)metaData.at(i-1).scanNum + (double)w*scaling/2.0;

			if(activeRight > finalLeft)
				plotMarkers[i-1]->setVisible(false);
			else
				plotMarkers[i-1]->setVisible(true);

			break;
		}
	}

	for(int i=0; i<plotCurveData.size();i++)
	{
		QVector<QPointF> d = plotCurveData.at(i);
		QVector<QPointF> filtered;

		if(d.isEmpty())
			return;

		//we could be scanning down!
		if(d.at(d.size()-1).x() > d.at(0).x())
		{
			//find first data point that is in the range of the plot
			int dataIndex = 0;
			while(dataIndex+1 < d.size() && d.at(dataIndex).x() < scaleMin)
				dataIndex++;

			//add the previous point to the filtered array
			//this will make sure the curve always goes to the edge of the plot
			if(dataIndex-1 >= 0)
				filtered.append(d.at(dataIndex-1));

			//at this point, dataIndex is at the first point within the range of the plot. loop over pixels, compressing data
			for(double pixel = firstPixel; pixel<lastPixel; pixel+=1.0)
			{
				double min = d.at(dataIndex).y(), max = min;
				int minIndex = dataIndex, maxIndex = dataIndex;
				double upperLimit = map.invTransform(pixel+1.0);
				int numPnts = 0;
				while(dataIndex+1 < d.size() && d.at(dataIndex).x() < upperLimit)
				{
					if(d.at(dataIndex).y() < min)
					{
						min = d.at(dataIndex).y();
						minIndex = dataIndex;
					}
					if(d.at(dataIndex).y() > max)
					{
						max = d.at(dataIndex).y();
						maxIndex = dataIndex;
					}
					dataIndex++;
					numPnts++;
				}
				if(numPnts == 1)
					filtered.append(d.at(dataIndex-1));
				else if (numPnts > 1)
				{
//					if(d.at(minIndex).x()<d.at(maxIndex).x())
//					{
                        filtered.append(QPointF(map.invTransform(pixel),d.at(minIndex).y()));
                        filtered.append(QPointF(map.invTransform(pixel+0.99),d.at(maxIndex).y()));
//					}
//					else
//					{
//                        filtered.append(QPointF(map.invTransform(pixel),d.at(maxIndex).y()));
//                        filtered.append(QPointF(map.invTransform(pixel+0.99),d.at(minIndex).y()));
//					}
				}
			}

			if(dataIndex < d.size())
				filtered.append(d.at(dataIndex));

		} //end scanning up section
		else
		{
			//find first data point that is in the range of the plot
			int dataIndex = d.size()-1;
			while(dataIndex-1 >= 0 && d.at(dataIndex).x() > scaleMax)
				dataIndex--;

			//add the previous point to the filtered array
			//this will make sure the curve always goes to the edge of the plot
			if(dataIndex+1 < d.size())
				filtered.append(d.at(dataIndex+1));

			//at this point, dataIndex is at the first point within the range of the plot. loop over pixels, compressing data
			for(double pixel = firstPixel; pixel<lastPixel; pixel+=1.0)
			{
				double min = d.at(dataIndex).y(), max = min;
				int minIndex = dataIndex, maxIndex = dataIndex;
				double upperLimit = map.invTransform(pixel+1.0);
				int numPnts = 0;
				while(dataIndex-1 >= 0 && d.at(dataIndex).x() < upperLimit)
				{
					if(d.at(dataIndex).y() < min)
					{
						min = d.at(dataIndex).y();
						minIndex = dataIndex;
					}
					if(d.at(dataIndex).y() > max)
					{
						max = d.at(dataIndex).y();
						maxIndex = dataIndex;
					}
					dataIndex--;
					numPnts++;
				}
				if(numPnts == 1)
					filtered.append(d.at(dataIndex+1));
				else if (numPnts > 1)
				{
//					if(d.at(minIndex).x()<d.at(maxIndex).x())
//					{
                        filtered.append(QPointF(map.invTransform(pixel),d.at(minIndex).y()));
                        filtered.append(QPointF(map.invTransform(pixel+0.99),d.at(maxIndex).y()));
//					}
//					else
//					{
//                        filtered.append(QPointF(map.invTransform(pixel),d.at(maxIndex).y()));
//                        filtered.append(QPointF(map.invTransform(pixel+0.99),d.at(minIndex).y()));
//					}
				}
			}

			if(dataIndex >= 0)
				filtered.append(d.at(dataIndex));
		}

		plotCurves[i]->setSamples(filtered);
	} //end loop over plotCurves

}

void BatchPlot::toggleCurve(QVariant item, bool hide, int index)
{
	Q_UNUSED(index)
	QwtPlotCurve *c = static_cast<QwtPlotCurve*>(infoToItem(item));
	if(c)
	{
		c->setVisible(!hide);
		replot();
	}
}

void BatchPlot::enableAutoScaling()
{
	autoScaleXActive = true;
	autoScaleYActive = true;
	replot();
}

void BatchPlot::printCallback()
{
	if(metaData.isEmpty() || plotCurves.isEmpty())
		return;

	switch(metaData.at(0).type)
	{
	case BatchManager::Survey:
		printSurvey();
		break;
	case BatchManager::DrScan:
		printDr();
		break;
	case BatchManager::Batch:
		printBatch();
		break;
    case BatchManager::Attenuation:
        printAttn();
        break;
	default:
		break;
    }
}

void BatchPlot::enableReplotting()
{
     d_replot = true;
	replot();
}

void BatchPlot::exportXY()
{
	if(plotCurveData.isEmpty())
		return;

	QString labelBase = title().text().toLower();
	labelBase.replace(QString(" "),QString(""));
	QString fileName = QFileDialog::getSaveFileName(this,QString("XY Export"),QString("~/%1.txt").arg(labelBase));

	if(fileName.isEmpty())
		return;

	QFile f(fileName);
	if(!f.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this,QString("File Error"),QString("Could not open selected file (%1) for writing. Please try again with a different filename.").arg(f.fileName()));
		return;
	}

	QTextStream t(&f);
	t.setRealNumberNotation(QTextStream::ScientificNotation);
	t.setRealNumberPrecision(12);
	QString tab("\t");
	QString nl("\n");

	t << QString("x%1").arg(labelBase);
	for(int i=0;i<plotCurveData.size();i++)
		t << tab << QString("y%1%2").arg(i).arg(labelBase);

	for(int i=0; i<plotCurveData.at(0).size(); i++)
	{
		t << nl << plotCurveData.at(0).at(i).x();
		for(int j=0; j<plotCurveData.size(); j++)
		{
			if(i < plotCurveData.at(j).size())
				t << tab << plotCurveData.at(j).at(i).y();
			else
				t << tab << 0.0;
		}
	}

	if(!calCurveData.isEmpty())
	{
		t << nl << nl << QString("calx%1").arg(labelBase) << tab << QString("caly%1").arg(labelBase);
		for(int i=0;i<calCurveData.size();i++)
			t << nl << calCurveData.at(i).x() << tab << calCurveData.at(i).y();
	}

    t.flush();
    f.close();
}

void BatchPlot::surveyDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
	if(metaData.isEmpty())
	{
		//this is the first chunk of data... initialize data storage
		calCurve = new QwtPlotCurve(QString("Calibration"));

		//color from settings...
		QColor highlight = QPalette().color(QPalette::Highlight);
		calCurve->setPen(QPen(highlight));
		calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(highlight),QPen(highlight),QSize(5,5)));

		//for a survey, there will only be one data curve
		QwtPlotCurve *curve = new QwtPlotCurve(QString("Survey"));
		curve->setPen(QPen(QPalette().color(QPalette::Text)));
		plotCurves.append(curve);

		calCurve->attach(this);
		curve->attach(this);

		surveyCalMinMaxes = QPair<double,double>(0.0,0.0);
		QList<QPair<double,double> > y;
		y.append(QPair<double,double>(0.0,0.0));
		yMinMaxes.append(y);

		plotCurveData.append(QVector<QPointF>());

	}

	if(md.isCal)
	{
		for(int i=calCurveData.size(); i<d.at(0).size(); i++)
		{
			surveyCalMinMaxes.first = qMin(surveyCalMinMaxes.first,d.at(0).at(i).y());
			surveyCalMinMaxes.second = qMax(surveyCalMinMaxes.second,d.at(0).at(i).y());
		}
		calCurveData = d.at(0);
		calCurve->setSamples(d.at(0));
	}
	else
	{
		for(int i=plotCurveData.at(0).size(); i<d.at(0).size(); i++)
		{
			yMinMaxes[0].first = qMin(yMinMaxes.at(0).first,d.at(0).at(i).y());
			yMinMaxes[0].second = qMax(yMinMaxes.at(0).second,d.at(0).at(i).y());
		}
		plotCurves[0]->setSamples(d.at(0));
		plotCurveData = d;
	}

}

void BatchPlot::drDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
	if(metaData.isEmpty())
	{
		QwtLegend *l = new QwtLegend;
		l->setDefaultItemMode(QwtLegendData::Checkable);
		l->installEventFilter(this);
		connect(l,&QwtLegend::checked,this,&BatchPlot::toggleCurve);
		insertLegend(l);

		QSettings s;
		s.beginGroup(QString("DrRanges"));
		for(int i=0; i<d.size(); i++)
		{
			QColor color = s.value(QString("DrRange%1").arg(QString::number(i)),
							   QPalette().color(QPalette::ToolTipBase)).value<QColor>();

			QwtPlotCurve *c = new QwtPlotCurve(QString("DrRange%1").arg(QString::number(i)));
			c->setPen(QPen(color));
//			c->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(color),QPen(color),QSize(5,5)));
			plotCurves.append(c);
			c->setLegendAttribute(QwtPlotCurve::LegendShowLine,true);
			c->setLegendAttribute(QwtPlotCurve::LegendShowSymbol,true);
			c->attach(this);

			QList<QPair<double,double> > y;
			y.append(QPair<double,double>(0.0,0.0));
			yMinMaxes.append(y);

			plotCurveData.append(QVector<QPointF>());

		}
		s.endGroup();
	}

	//only add the data if this is not a calibration!
	if(!md.isCal)
	{
		for(int i=0; i<d.size() && i<plotCurves.size(); i++)
		{
			for(int j=plotCurveData.at(i).size(); j<d.at(i).size(); j++)
			{
				yMinMaxes[i].first = qMin(yMinMaxes.at(i).first,d.at(i).at(j).y());
				yMinMaxes[i].second = qMax(yMinMaxes.at(i).second,d.at(i).at(j).y());
			}
			plotCurves[i]->setSamples(d.at(i));
		}
		plotCurveData = d;
	}

}

void BatchPlot::batchDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
	if(metaData.isEmpty())
	{
		//for a batch, there will only be one data curve
		QwtPlotCurve *curve = new QwtPlotCurve(QString("Batch Data"));
		curve->setPen(QPen(QPalette().color(QPalette::Text)));
		plotCurves.append(curve);

		curve->attach(this);

		QList<QPair<double,double> > y;
		y.append(QPair<double,double>(0.0,0.0));
		yMinMaxes.append(y);

		plotCurveData.append(QVector<QPointF>());
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
    if(!d_hideBatchLabels)
        m->attach(this);
	plotMarkers.append(m);

	for(int i=plotCurveData.at(0).size(); i<d.at(0).size(); i++)
	{
		yMinMaxes[0].first = qMin(yMinMaxes.at(0).first,d.at(0).at(i).y());
		yMinMaxes[0].second = qMax(yMinMaxes.at(0).second,d.at(0).at(i).y());
	}

	plotCurves[0]->setSamples(d.at(0));
    plotCurveData = d;
}

void BatchPlot::attnDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
    if(metaData.isEmpty())
    {
        QwtLegend *l = new QwtLegend;
        l->setDefaultItemMode(QwtLegendData::Checkable);
        l->installEventFilter(this);
	   connect(l,&QwtLegend::checked,this,&BatchPlot::toggleCurve);
        insertLegend(l);

        //There are 4 data curves for an attenuation scan
        QSettings s;
        s.beginGroup(QString("AttnCurves"));
        QStringList sl;
        sl << QString("TuningAttenuation") << QString("TuningVoltage") << QString("CorrectedAttenuation") << QString("CorrectedVoltage");
        for(int i=0; i<sl.size(); i++)
        {
            QColor color = s.value(sl.at(i),QPalette().color(QPalette::ToolTipBase)).value<QColor>();
            QwtPlotCurve *c = new QwtPlotCurve(sl.at(i));
            c->setPen(QPen(color));
//			c->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(color),QPen(color),QSize(5,5)));
            plotCurves.append(c);
            c->setLegendAttribute(QwtPlotCurve::LegendShowLine,true);
            c->setLegendAttribute(QwtPlotCurve::LegendShowSymbol,true);
            c->attach(this);

            QList<QPair<double,double> > y;
            y.append(QPair<double,double>(0.0,0.0));
            yMinMaxes.append(y);

            plotCurveData.append(QVector<QPointF>());

        }
        s.endGroup();
    }

    //recalculate min and max values for curves
    //note that we only need to compare new data, which is either at the beginning or end of d, depending on if scan is going up or down
    for(int i=0; i<d.size() && i<plotCurves.size(); i++)
    {
        if(metaData.isEmpty() || md.maxXVal>metaData.at(metaData.size()-1).maxXVal)
        {
            for(int j=plotCurveData.at(i).size(); j<d.at(i).size(); j++)
            {
                yMinMaxes[i].first = qMin(yMinMaxes.at(i).first,d.at(i).at(j).y());
                yMinMaxes[i].second = qMax(yMinMaxes.at(i).second,d.at(i).at(j).y());
            }
        }
        else
        {
            for(int j=0; j<d.at(i).size() - plotCurveData.at(i).size(); j++)
            {
                yMinMaxes[i].first = qMin(yMinMaxes.at(i).first,d.at(i).at(j).y());
                yMinMaxes[i].second = qMax(yMinMaxes.at(i).second,d.at(i).at(j).y());
            }
        }
        plotCurves[i]->setSamples(d.at(i));
    }
    //set curve data
    plotCurveData = d;
}

bool BatchPlot::eventFilter(QObject *obj, QEvent *ev)
{
	if(obj == static_cast<QObject*>(legend()))
	{
		if(ev->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *me = static_cast<QMouseEvent*>(ev);
			if(me && me->button() == Qt::RightButton)
			{
				//here's the hook for changing color
				QwtLegend *l = static_cast<QwtLegend*>(legend());
				QVariant item = l->itemInfo(l->contentsWidget()->childAt(me->pos()));
				QwtPlotCurve *c = static_cast<QwtPlotCurve*>(infoToItem(item));
				if(!c)
				{
					ev->ignore();
					return true;
				}
				QColor color = QColorDialog::getColor(c->pen().color());
				if(color.isValid())
				{
					QSettings s;
                    if(c->title().text().startsWith(QString("Dr")))
                        s.beginGroup(QString("DrRanges"));
                    else
                        s.beginGroup(QString("AttnCurves"));
					c->setPen(QPen(color));
//					c->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(color),QPen(color),QSize(5,5)));
					QString str = c->title().text();
					s.setValue(str,color);
					emit colorChanged(str,color);
					replot();
					s.endGroup();
					s.sync();
				}

				ev->accept();
				return true;
			}
		}
	}	
	else if (obj == static_cast<QObject*>(this->canvas()))
	{
		if(ev->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *me = static_cast<QMouseEvent*>(ev);
			if(me && me->button() == Qt::MiddleButton)
			{
				if(!autoScaleXActive || !autoScaleYActive)
				{
					panning = true;
					panClickPos = me->pos();
					ev->accept();
					return true;
				}
			}
		}
		else if (ev->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent *me = static_cast<QMouseEvent*>(ev);
			if(me->button() == Qt::MiddleButton)
			{
				panning = false;
				ev->accept();
				return true;
			}
			else if(me->button() == Qt::LeftButton && (me->modifiers()|Qt::ControlModifier))
			{
				enableAutoScaling();
				ev->accept();
				return true;
			}
		}
		else if (panning && ev->type() == QEvent::MouseMove)
		{
			pan(static_cast<QMouseEvent*>(ev));
			ev->accept();
			return true;
		}
		else if (ev->type() == QEvent::Wheel)
		{
			zoom(static_cast<QWheelEvent*>(ev));
			ev->accept();
			return true;
		}
	}

	return QwtPlot::eventFilter(obj,ev);
}

void BatchPlot::replot()
{
	autoScale();
	filterData();
	QwtPlot::replot();
}

void BatchPlot::pan(QMouseEvent *me)
{
	if(!me)
		return;

	double xScaleMin = axisScaleDiv(QwtPlot::xBottom).lowerBound();
	double xScaleMax = axisScaleDiv(QwtPlot::xBottom).upperBound();
	double yScaleMin = axisScaleDiv(QwtPlot::yLeft).lowerBound();
	double yScaleMax = axisScaleDiv(QwtPlot::yLeft).upperBound();

	QPoint delta = panClickPos - me->pos();
	double dx = (xScaleMax-xScaleMin)/(double)canvas()->width()*delta.x();
	double dy = (yScaleMax-yScaleMin)/(double)canvas()->height()*-delta.y();


	if(xScaleMin + dx < autoScaleXRange.first)
		dx = autoScaleXRange.first - xScaleMin;
	if(xScaleMax + dx > autoScaleXRange.second)
		dx = autoScaleXRange.second - xScaleMax;
	if(yScaleMin + dy < autoScaleYRange.first)
		dy = autoScaleYRange.first - yScaleMin;
	if(yScaleMax + dy > autoScaleYRange.second)
		dy = autoScaleYRange.second - yScaleMax;

	panClickPos -= delta;
	setAxisScale(QwtPlot::xBottom,xScaleMin + dx,xScaleMax + dx);
	setAxisScale(QwtPlot::yLeft,yScaleMin + dy,yScaleMax + dy);

	replot();
}

void BatchPlot::zoom(QWheelEvent *we)
{
	if(!we)
		return;

	//ctrl-wheel: zoom only horizontally
	//shift-wheel: zoom only vertically
	//no modifiers: zoom both

	//one step, which is 15 degrees, will zoom 10%
	//the delta function is in units of 1/8th a degree
	int numSteps = we->delta()/8/15;

	if(!(we->modifiers() & Qt::ShiftModifier))
	{
		//do horizontal rescaling
		//get current scale
		double scaleXMin = axisScaleDiv(QwtPlot::xBottom).lowerBound();
		double scaleXMax = axisScaleDiv(QwtPlot::xBottom).upperBound();

		//find mouse position on scale (and make sure it's on the scale!)
		double mousePos = canvasMap(QwtPlot::xBottom).invTransform(we->pos().x());
		mousePos = qMax(mousePos,scaleXMin);
		mousePos = qMin(mousePos,scaleXMax);

		//calculate distances from mouse to scale; remove or add 10% from/to each
		scaleXMin += fabs(mousePos - scaleXMin)*0.1*(double)numSteps;
		scaleXMax -= fabs(mousePos - scaleXMax)*0.1*(double)numSteps;

		//we can't go beyond range set by autoscale limits
		scaleXMin = qMax(scaleXMin,autoScaleXRange.first);
		scaleXMax = qMin(scaleXMax,autoScaleXRange.second);

		if(scaleXMin <= autoScaleXRange.first && scaleXMax >= autoScaleXRange.second)
			autoScaleXActive = true;
		else
		{
			autoScaleXActive = false;
			setAxisScale(QwtPlot::xBottom,scaleXMin,scaleXMax);
		}
	}

	if(!(we->modifiers() & Qt::ControlModifier))
	{
		//do horizontal rescaling
		//get current scale
		double scaleYMin = axisScaleDiv(QwtPlot::yLeft).lowerBound();
		double scaleYMax = axisScaleDiv(QwtPlot::yLeft).upperBound();

		//find mouse position on scale (and make sure it's on the scale!)
		double mousePos = canvasMap(QwtPlot::yLeft).invTransform(we->pos().y());
		mousePos = qMax(mousePos,scaleYMin);
		mousePos = qMin(mousePos,scaleYMax);

		//calculate distances from mouse to scale; remove or add 10% from/to each
		scaleYMin += fabs(mousePos - scaleYMin)*0.1*(double)numSteps;
		scaleYMax -= fabs(mousePos - scaleYMax)*0.1*(double)numSteps;

		//we can't go beyond range set by autoscale limits
		scaleYMin = qMax(scaleYMin,autoScaleYRange.first);
		scaleYMax = qMin(scaleYMax,autoScaleYRange.second);

		if(scaleYMin <= autoScaleYRange.first && scaleYMax >= autoScaleYRange.second)
			autoScaleYActive = true;
		else
		{
			autoScaleYActive = false;
			setAxisScale(QwtPlot::yLeft,scaleYMin,scaleYMax);
		}
	}

	replot();
}

void BatchPlot::autoScale()
{
	if(metaData.isEmpty() || yMinMaxes.isEmpty())
		return;

	QPair<double,double> yVals(0.0,0.0);
	QPair<double,double> xVals(0.0,0.0);

	//x axis... first do data, then do zone, if visible
	xVals.first = qMin(metaData.at(0).minXVal,metaData.at(metaData.size()-1).minXVal);
	xVals.second = qMax(metaData.at(0).maxXVal,metaData.at(metaData.size()-1).maxXVal);

    if(zone->plot() == this)
    {
        if(metaData.at(0).type == BatchManager::Survey && autoScaleXActive)
        {
            setAxisScale(QwtPlot::xBottom,xVals.first,xVals.second);
            updateAxes();
            //updateAxes() doesn't reprocess the scale labels, which might need to be recalculated.
            //this line will take care of that and avoid plot glitches
            QApplication::sendPostedEvents(this,QEvent::LayoutRequest);
            recalcZone(false);
        }
        xVals.first = qMin(xVals.first,zone->interval().minValue());
        xVals.second = qMax(xVals.second,zone->interval().maxValue());
    }

	//get y min and max (min will almost certainly remain 0)
	for(int i=0; i<yMinMaxes.size(); i++)
	{
		if(plotCurves.at(i)->isVisible())
		{
			yVals.first = qMin(yVals.first,yMinMaxes.at(i).first);
			yVals.second = qMax(yVals.second,yMinMaxes.at(i).second);
		}
	}
	if(metaData.at(0).type == BatchManager::Survey)
	{
		if(calCurve->isVisible())
		{
			yVals.first = qMin(yVals.first,surveyCalMinMaxes.first);
			yVals.second = qMax(yVals.second,surveyCalMinMaxes.second);
		}
	}

	//Add extra vertical space for labels
    if(metaData.at(0).type == BatchManager::Batch && !d_hideBatchLabels)
	{
		int height = 0;
		QFontMetrics fm(plotMarkers.at(0)->label().font());
		for(int i=0;i<plotMarkers.size();i++)
		{
			QString text = plotMarkers.at(i)->label().text();
			int numLines = text.split(QString("\n"),QString::SkipEmptyParts).size();
			height = qMax(height,fm.boundingRect(text).height()*numLines);
		}

		//height is in pixels. We need to convert that number of pixels into plot coordinates, and add to vertical scale
		double scaling = (yVals.second - yVals.first)/(double)canvas()->height();
		yVals.second += height*scaling;

		//in batch mode with autoscaling active, the first and last labels will always be visible
		//so make sure they have horizontal space

		//we need the widest line in the label
		int firstLabelWidth = 0;
		QStringList lines = plotMarkers.at(0)->label().text().split(QString("\n"),QString::SkipEmptyParts);
		for(int i=0; i<lines.size(); i++)
			firstLabelWidth = qMax(firstLabelWidth,fm.boundingRect(lines.at(i)).width());

		int lastLabelWidth = 0;
		lines = plotMarkers.at(plotMarkers.size()-1)->label().text().split(QString("\n"),QString::SkipEmptyParts);
		for(int i=0;i<lines.size();i++)
			lastLabelWidth = qMax(lastLabelWidth,fm.boundingRect(lines.at(i)).width());

		int firstLabelPos = metaData.at(0).scanNum;
		int lastLabelPos = metaData.at(metaData.size()-1).scanNum;
		scaling = (xVals.second - xVals.first)/(double)canvas()->width();

		xVals.first = qMin(xVals.first,(double)firstLabelPos-(double)firstLabelWidth*scaling/2.0);
		xVals.second = qMax(xVals.second,(double)lastLabelPos+(double)lastLabelWidth*scaling/2.0);
	}

	autoScaleYRange = yVals;
	autoScaleXRange = xVals;

	if(autoScaleXActive)
		setAxisScale(QwtPlot::xBottom,xVals.first,xVals.second);
	if(autoScaleYActive)
		setAxisScale(QwtPlot::yLeft,yVals.first,yVals.second);
}

void BatchPlot::resizeEvent(QResizeEvent *e)
{
	autoScale();
	filterData();
	QwtPlot::resizeEvent(e);
}

void BatchPlot::printSurvey()
{
	//set up options dialog
	QDialog d;
	d.setWindowTitle(QString("Survey Printing Options"));

	//graph-related options
	QGroupBox *graphBox = new QGroupBox(QString("Graph setup"),&d);
	QFormLayout *gbl = new QFormLayout(graphBox);

	QSpinBox *graphsPerPageBox = new QSpinBox(graphBox);
	graphsPerPageBox->setMinimum(1);
	graphsPerPageBox->setMaximum(4);
	graphsPerPageBox->setValue(3);
	graphsPerPageBox->setToolTip(QString("Number of graphs per page"));
	gbl->addRow(QString("Graphs per page"),graphsPerPageBox);

	QDoubleSpinBox *xMinBox = new QDoubleSpinBox(graphBox);
	xMinBox->setToolTip(QString("Starting frequency"));
	xMinBox->setMinimum(autoScaleXRange.first);
	xMinBox->setMaximum(autoScaleXRange.second);
	xMinBox->setValue(autoScaleXRange.first);
	xMinBox->setSingleStep(5.0);
	gbl->addRow(QString("X min"),xMinBox);

	QDoubleSpinBox *xMaxBox = new QDoubleSpinBox(graphBox);
	xMaxBox->setToolTip(QString("Ending frequency"));
	xMaxBox->setMinimum(autoScaleXRange.first);
	xMaxBox->setMaximum(autoScaleXRange.second);
	xMaxBox->setValue(autoScaleXRange.second);
	xMaxBox->setSingleStep(5.0);
	gbl->addRow(QString("X max"),xMaxBox);

	QDoubleSpinBox *widthPerGraphBox = new QDoubleSpinBox(graphBox);
	widthPerGraphBox->setToolTip(QString("Frequency span of x axis on each graph"));
	widthPerGraphBox->setMinimum(0.1);
	widthPerGraphBox->setMaximum(100.0);
	widthPerGraphBox->setValue(10.0);
	widthPerGraphBox->setSingleStep(5.0);
	widthPerGraphBox->setSuffix(QString(" MHz"));
	gbl->addRow(QString("X axis span"),widthPerGraphBox);

	QDoubleSpinBox *yMinBox = new QDoubleSpinBox(graphBox);
	yMinBox->setToolTip(QString("Minimum value on y axis of each graph"));
    yMinBox->setDecimals(3);
	yMinBox->setMinimum(0.0);
	yMinBox->setMaximum(autoScaleYRange.second*0.9);
	yMinBox->setValue(0.0);
	yMinBox->setSingleStep(autoScaleYRange.second*0.1);
	gbl->addRow(QString("Y min"),yMinBox);

	QDoubleSpinBox *yMaxBox = new QDoubleSpinBox(graphBox);
	yMaxBox->setToolTip(QString("Maximum value on y axis of each graph"));
    yMaxBox->setDecimals(3);
	yMaxBox->setMinimum(0.0);
	yMaxBox->setMaximum(autoScaleYRange.second);
	yMaxBox->setValue(autoScaleYRange.second);
	yMaxBox->setSingleStep(autoScaleYRange.second*0.1);
	gbl->addRow(QString("Y max"),yMaxBox);

	graphBox->setLayout(gbl);

	//survey-related options
	QGroupBox *optionsBox = new QGroupBox("Options",&d);
	QFormLayout *obl = new QFormLayout(optionsBox);

	QCheckBox *colorCheckBox  = new QCheckBox(optionsBox);
	colorCheckBox->setToolTip(QString("If checked, graphs will be printed in color. The program will attempt to\ndarken any light foreground colors to provide good contrast on paper"));
	colorCheckBox->setChecked(false);
	obl->addRow(QString("Color"),colorCheckBox);

	QCheckBox *printCalBox = new QCheckBox(optionsBox);
	printCalBox->setToolTip(QString("If checked, the calibration scan (if present) will also be printed on the right axis"));
	printCalBox->setChecked(true);
	obl->addRow(QString("Print calibration"),printCalBox);

	QCheckBox *scanNumbersBox = new QCheckBox(optionsBox);
	scanNumbersBox->setToolTip(QString("Show scan numbers on graphs"));
	scanNumbersBox->setChecked(true);
	obl->addRow(QString("Show scan numbers"),scanNumbersBox);

	QCheckBox *gridCheckBox = new QCheckBox(optionsBox);
	gridCheckBox->setToolTip(QString("Show a grid on the graph canvas"));
	gridCheckBox->setChecked(true);
	obl->addRow(QString("Grid"),gridCheckBox);

	optionsBox->setLayout(obl);

	QHBoxLayout *dl = new QHBoxLayout();
	dl->addWidget(graphBox);
	dl->addWidget(optionsBox);

	QVBoxLayout *vl = new QVBoxLayout(&d);
	vl->addLayout(dl);

	QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal,&d);
	connect(box->button(QDialogButtonBox::Ok),&QAbstractButton::clicked,&d,&QDialog::accept);
	connect(box->button(QDialogButtonBox::Cancel),&QAbstractButton::clicked,&d,&QDialog::reject);
	vl->addWidget(box);

	d.setLayout(vl);

	int ret = d.exec();
	if(ret != QDialog::Accepted)
		return;


	//launch print dialog
	QPrinter p(QPrinter::HighResolution);
	p.setOrientation(QPrinter::Landscape);
	p.setCreator(QString("QtFTM"));
	QPrintDialog pd(&p);

	if(!pd.exec())
		return;

	//show busy cursor during printing
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//configure plot for printing
	//first, override any invalid settings, and calculate number of pages
	double xMin = xMinBox->value(), xMax = xMaxBox->value(), yMin = yMinBox->value(),
			yMax = yMaxBox->value(), xWidth = widthPerGraphBox->value();
	int perPage = graphsPerPageBox->value();

	if(xMin == xMax)
	{
		xMin = autoScaleXRange.first;
		xMax = autoScaleXRange.second;
	}
	if(xMin > xMax)
		qSwap(xMin,xMax);

	if(yMin == yMax)
	{
		yMin = autoScaleYRange.first;
		yMax = autoScaleYRange.second;
	}
	if(yMin > yMax)
		qSwap(yMin,yMax);

	//store pens
	QPen surveyPen(plotCurves.at(0)->pen());
	QPen calPen;
	if(!calCurveData.isEmpty())
		calPen = QPen(calCurve->pen());

	//survey curve settings
	//survey pen is always black
	plotCurves[0]->setPen(QPen(Qt::black,1.0));
	plotCurves[0]->setRenderHint(QwtPlotItem::RenderAntialiased);

	//cal curve settings
	if(!calCurveData.isEmpty() && printCalBox->isChecked())
	{
		calCurve->setAxes(QwtPlot::xBottom,QwtPlot::yRight);
		calCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
		enableAxis(QwtPlot::yRight,true);
		setAxisAutoScale(QwtPlot::yRight,false);
		setAxisScale(QwtPlot::yRight,0.0,surveyCalMinMaxes.second);

		//if printing in color, make sure the color is darkish
		if(colorCheckBox->isChecked())
		{
			QColor c = calPen.color();
			if(c.red()>128)
				c.setRed(c.red()-128);
			if(c.blue()>128)
				c.setBlue(c.blue()-128);
			if(c.green()>128)
				c.setGreen(c.green()-128);

			calCurve->setPen(QPen(c,1.0));
			calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(c),QPen(c,1.0),QSize(5,5)));
		}
		else //otherwise, it's black
		{
			calCurve->setPen(Qt::black,1.0);
			calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(Qt::black),QPen(Qt::black,1.0),QSize(5,5)));
		}
	}
	else if(!calCurveData.isEmpty())
		calCurve->setVisible(false);

	//grid
	if(gridCheckBox->isChecked())
	{
		QwtPlotGrid *pGrid = new QwtPlotGrid;
		pGrid->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
		pGrid->setMajorPen(QPen(QBrush(Qt::gray),1.0,Qt::DashLine));
		pGrid->setMinorPen(QPen(QBrush(Qt::lightGray),1.0,Qt::DotLine));
		pGrid->enableXMin(true);
		pGrid->enableYMin(true);
		pGrid->setRenderHint(QwtPlotItem::RenderAntialiased);
		pGrid->attach(this);
	}

	//scan number markers
	if(scanNumbersBox->isChecked())
	{
		QFont f(QString("sans-serif"),7);
		QFontMetrics fm(f);

		double scaling = xWidth/(double)width();
		double labelWidth = (double)fm.boundingRect(QString("99999999")).width();
		double scanWidth = 1.0;
		int blah=0;
		while(blah < metaData.size())
		{
			if(metaData.at(blah).isCal)
			{
				blah++;
				continue;
			}
			scanWidth = metaData.at(blah).maxXVal-metaData.at(blah).minXVal;
			break;
		}
		labelWidth*= scaling;
		int labelSpacingSteps = ceil(1.5*labelWidth/scanWidth);
		for(int i=0; i<metaData.size(); i+=labelSpacingSteps)
		{
			if(metaData.at(i).isCal)
            {
				i++;
                if(i>=metaData.size())
                    break;
            }
			QwtText t(QString::number(metaData.at(i).scanNum));
			t.setFont(f);
			t.setColor(Qt::black);
			t.setBackgroundBrush(QBrush(Qt::white));
			double pos = (metaData.at(i).maxXVal+metaData.at(i).minXVal)/2.0;
			QwtPlotMarker *m = new QwtPlotMarker();
			m->setRenderHint(QwtPlotItem::RenderAntialiased);
			m->setLineStyle(QwtPlotMarker::VLine);
			QColor c(Qt::black);
			c.setAlpha(0);
			m->setLinePen(QPen(c));
			m->setLabel(t);
			m->setLabelAlignment(Qt::AlignTop|Qt::AlignHCenter);
			m->setXValue(pos);
			m->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
			m->attach(this);
		}
	}

	//set y range
	//store plot ranges
	QPair<double,double> xScale;
	xScale.first = axisScaleDiv(QwtPlot::xBottom).lowerBound();
	xScale.second = axisScaleDiv(QwtPlot::xBottom).upperBound();

	QPair<double,double> yScale;
	yScale.first = axisScaleDiv(QwtPlot::yLeft).lowerBound();
	yScale.second = axisScaleDiv(QwtPlot::yLeft).upperBound();
	setAxisScale(QwtPlot::yLeft,yMin,yMax);

	//un-filter data
	plotCurves[0]->setSamples(plotCurveData.at(0));

	//hide zone
	bool zoneWasVisible = zone->isVisible();
	if(zoneWasVisible)
		zone->setVisible(false);

	//recolor axes
	QPalette pal;
	pal.setColor(QPalette::Text,QColor(Qt::black));
	pal.setColor(QPalette::Foreground,QColor(Qt::black));
	axisWidget(QwtPlot::xBottom)->setPalette(pal);
	axisWidget(QwtPlot::yLeft)->setPalette(pal);
	axisWidget(QwtPlot::yRight)->setPalette(pal);

	//do the printing
    QwtText t = title();
    QwtText oldTitle = title();
    t = QwtText(QString("FTM%1 %2").arg(QTFTM_SPECTROMETER).arg(title().text()));

    setTitle(QString(""));
    doPrint(xMin,xMax,xWidth,perPage,t.text(),&p);

	//now, undo everything
    setTitle(oldTitle);

	//restore axis colors
	axisWidget(QwtPlot::xBottom)->setPalette(QPalette());
	axisWidget(QwtPlot::yLeft)->setPalette(QPalette());
	axisWidget(QwtPlot::yRight)->setPalette(QPalette());

	//restore scales
	setAxisScale(QwtPlot::xBottom,xScale.first,xScale.second);
	setAxisScale(QwtPlot::yLeft,yScale.first,yScale.second);

	//remove markers and grid (this will delete them all if they're there)
	detachItems(QwtPlotItem::Rtti_PlotGrid);
	detachItems(QwtPlotItem::Rtti_PlotMarker);

	//restore curves
	if(zoneWasVisible)
		zone->setVisible(true);

	plotCurves[0]->setPen(surveyPen);
	plotCurves[0]->setRenderHint(QwtPlotItem::RenderAntialiased,false);
	enableAxis(QwtPlot::yRight,false);
	if(!calCurveData.isEmpty())
	{
		calCurve->setPen(calPen);
		calCurve->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
		calCurve->setVisible(true);
		calCurve->setRenderHint(QwtPlotItem::RenderAntialiased,false);
		calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(calPen.color()),QPen(calPen.color(),1.0),QSize(5,5)));
	}

	replot();

	QApplication::restoreOverrideCursor();

}

void BatchPlot::printDr()
{
	//set up options dialog
	QDialog d;
	d.setWindowTitle(QString("DR Scan Printing Options"));

	//graph-related options
	QGroupBox *graphBox = new QGroupBox(QString("Graph setup"),&d);
	QFormLayout *gbl = new QFormLayout(graphBox);

	QSpinBox *graphsPerPageBox = new QSpinBox(graphBox);
	graphsPerPageBox->setMinimum(1);
	graphsPerPageBox->setMaximum(4);
	graphsPerPageBox->setValue(3);
	graphsPerPageBox->setToolTip(QString("Number of graphs per page"));
	gbl->addRow(QString("Graphs per page"),graphsPerPageBox);

	QDoubleSpinBox *xMinBox = new QDoubleSpinBox(graphBox);
	xMinBox->setToolTip(QString("Starting frequency"));
	xMinBox->setMinimum(autoScaleXRange.first);
	xMinBox->setMaximum(autoScaleXRange.second);
	xMinBox->setValue(autoScaleXRange.first);
	xMinBox->setSingleStep(5.0);
	gbl->addRow(QString("X min"),xMinBox);

	QDoubleSpinBox *xMaxBox = new QDoubleSpinBox(graphBox);
	xMaxBox->setToolTip(QString("Ending frequency"));
	xMaxBox->setMinimum(autoScaleXRange.first);
	xMaxBox->setMaximum(autoScaleXRange.second);
	xMaxBox->setValue(autoScaleXRange.second);
	xMaxBox->setSingleStep(5.0);
	gbl->addRow(QString("X max"),xMaxBox);

	QDoubleSpinBox *widthPerGraphBox = new QDoubleSpinBox(graphBox);
	widthPerGraphBox->setToolTip(QString("Frequency span of x axis on each graph"));
	widthPerGraphBox->setMinimum(1.0);
	widthPerGraphBox->setMaximum(100.0);
	widthPerGraphBox->setValue(5.0);
	widthPerGraphBox->setSingleStep(1.0);
	widthPerGraphBox->setSuffix(QString(" MHz"));
	gbl->addRow(QString("X axis span"),widthPerGraphBox);

	QDoubleSpinBox *yMinBox = new QDoubleSpinBox(graphBox);
	yMinBox->setToolTip(QString("Minimum value on y axis of each graph"));
    yMinBox->setDecimals(3);
	yMinBox->setMinimum(0.0);
	yMinBox->setMaximum(autoScaleYRange.second*0.9);
	yMinBox->setValue(0.0);
	yMinBox->setSingleStep(autoScaleYRange.second*0.1);
	gbl->addRow(QString("Y min"),yMinBox);

	QDoubleSpinBox *yMaxBox = new QDoubleSpinBox(graphBox);
	yMaxBox->setToolTip(QString("Maximum value on y axis of each graph"));
    yMaxBox->setDecimals(3);
	yMaxBox->setMinimum(0.0);
	yMaxBox->setMaximum(autoScaleYRange.second);
	yMaxBox->setValue(autoScaleYRange.second);
	yMaxBox->setSingleStep(autoScaleYRange.second*0.1);
	gbl->addRow(QString("Y max"),yMaxBox);

	graphBox->setLayout(gbl);

	//dr-related options
	QGroupBox *optionsBox = new QGroupBox("Options",&d);
	QFormLayout *obl = new QFormLayout(optionsBox);

	QCheckBox *colorCheckBox  = new QCheckBox(optionsBox);
	colorCheckBox->setToolTip(QString("If checked, graphs will be printed in color. The program will attempt to\ndarken any light foreground colors to provide good contrast on paper"));
	colorCheckBox->setChecked(false);
	obl->addRow(QString("Color"),colorCheckBox);

	QCheckBox *ownGraphsBox = new QCheckBox(optionsBox);
	ownGraphsBox->setToolTip(QString("If checked, the calibration scan (if present) will also be printed on the right axis"));
	ownGraphsBox->setChecked(true);
	if(plotCurves.size()==1)
		ownGraphsBox->setEnabled(false);
	obl->addRow(QString("1 range per graph"),ownGraphsBox);

	QCheckBox *scanNumbersBox = new QCheckBox(optionsBox);
	scanNumbersBox->setToolTip(QString("Show scan numbers on graphs"));
	scanNumbersBox->setChecked(true);
	obl->addRow(QString("Show scan numbers"),scanNumbersBox);

	QCheckBox *gridCheckBox = new QCheckBox(optionsBox);
	gridCheckBox->setToolTip(QString("Show a grid on the graph canvas"));
	gridCheckBox->setChecked(true);
	obl->addRow(QString("Grid"),gridCheckBox);

	optionsBox->setLayout(obl);

	QHBoxLayout *dl = new QHBoxLayout();
	dl->addWidget(graphBox);
	dl->addWidget(optionsBox);

	QVBoxLayout *vl = new QVBoxLayout(&d);
	vl->addLayout(dl);

	QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal,&d);
	connect(box->button(QDialogButtonBox::Ok),&QAbstractButton::clicked,&d,&QDialog::accept);
	connect(box->button(QDialogButtonBox::Cancel),&QAbstractButton::clicked,&d,&QDialog::reject);
	vl->addWidget(box);

	d.setLayout(vl);

	int ret = d.exec();
	if(ret != QDialog::Accepted)
		return;


	//launch print dialog
    QPrinter p(QPrinter::HighResolution);
	p.setOrientation(QPrinter::Landscape);
	p.setCreator(QString("QtFTM"));
	QPrintDialog pd(&p);

	if(!pd.exec())
		return;

	//show busy cursor during printing
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//configure plot for printing
	//first, override any invalid settings, and calculate number of pages
	double xMin = xMinBox->value(), xMax = xMaxBox->value(), yMin = yMinBox->value(),
			yMax = yMaxBox->value(), xWidth = widthPerGraphBox->value();
	int perPage = graphsPerPageBox->value();

	if(xMin == xMax)
	{
		xMin = autoScaleXRange.first;
		xMax = autoScaleXRange.second;
	}
	if(xMin > xMax)
		qSwap(xMin,xMax);

	if(yMin == yMax)
	{
		yMin = autoScaleYRange.first;
		yMax = autoScaleYRange.second;
	}
	if(yMin > yMax)
		qSwap(yMin,yMax);

	//store and set pens
	QList<QPen> pens;
	for(int i=0; i<plotCurves.size(); i++)
	{
		pens.append(plotCurves.at(i)->pen());
		//if printing in color, make sure the color is darkish
		if(colorCheckBox->isChecked())
		{
			QColor c = pens.at(i).color();
			if(c.red()>128)
				c.setRed(c.red()-128);
			if(c.blue()>128)
				c.setBlue(c.blue()-128);
			if(c.green()>128)
				c.setGreen(c.green()-128);

			plotCurves[i]->setPen(QPen(c,1.0));
		}
		else //otherwise, it's black
		{
			plotCurves[i]->setPen(Qt::black,1.0);
		}
		plotCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased,true);
		plotCurves[i]->setSamples(plotCurveData.at(i));
	}

	//grid
	if(gridCheckBox->isChecked())
	{
		QwtPlotGrid *pGrid = new QwtPlotGrid;
		pGrid->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
		pGrid->setMajorPen(QPen(QBrush(Qt::gray),1.0,Qt::DashLine));
		pGrid->setMinorPen(QPen(QBrush(Qt::lightGray),1.0,Qt::DotLine));
		pGrid->enableXMin(true);
		pGrid->enableYMin(true);
		pGrid->setRenderHint(QwtPlotItem::RenderAntialiased);
		pGrid->attach(this);
	}

	//scan number markers
	if(scanNumbersBox->isChecked())
	{
		QFont f(QString("sans-serif"),7);
		QFontMetrics fm(f);

		double scaling = xWidth/(double)width();
		double labelWidth = (double)fm.boundingRect(QString("99999999")).width();
		labelWidth*= scaling;

//		if(ownGraphsBox->isChecked())
//			labelWidth*=plotCurves.size();
		int labelSpacingSteps = 1;
		if(metaData.size()>4)
		{
			//calculate step size. keep in mind that cal scans (taken between points) aren't plotted!
			double stepSize = 0.0;
			if(metaData.at(0).isCal)
				stepSize = (metaData.at(3).maxXVal-metaData.at(1).maxXVal)/2.0;
			else
				stepSize = metaData.at(1).maxXVal-metaData.at(0).maxXVal;

			//calculate number of steps needed before labels won't overlap
			labelSpacingSteps = (int)ceil(labelWidth/stepSize) + 1;
		}
		for(int i=0; i<metaData.size(); i+=labelSpacingSteps)
		{
			if(metaData.at(i).isCal)
				i++;
			QwtText t(QString::number(metaData.at(i).scanNum));
			t.setFont(f);
			t.setColor(Qt::black);
			t.setBackgroundBrush(QBrush(Qt::white));
			double pos = (metaData.at(i).maxXVal+metaData.at(i).minXVal)/2.0;
			QwtPlotMarker *m = new QwtPlotMarker();
			m->setRenderHint(QwtPlotItem::RenderAntialiased);
			m->setLineStyle(QwtPlotMarker::VLine);
			QColor c(Qt::black);
			c.setAlpha(0);
			m->setLinePen(QPen(c));
			m->setLabel(t);
			m->setLabelAlignment(Qt::AlignBottom|Qt::AlignHCenter);
			m->setXValue(pos);
			m->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
			m->attach(this);
		}
	}

	//set y range
	//store plot ranges
	QPair<double,double> xScale;
	xScale.first = axisScaleDiv(QwtPlot::xBottom).lowerBound();
	xScale.second = axisScaleDiv(QwtPlot::xBottom).upperBound();

	QPair<double,double> yScale;
	yScale.first = axisScaleDiv(QwtPlot::yLeft).lowerBound();
	yScale.second = axisScaleDiv(QwtPlot::yLeft).upperBound();
	setAxisScale(QwtPlot::yLeft,yMin,yMax);

	//hide zone
	bool zoneWasVisible = zone->isVisible();
	if(zoneWasVisible)
		zone->setVisible(false);

	//recolor axes
	QPalette pal;
	pal.setColor(QPalette::Text,QColor(Qt::black));
	pal.setColor(QPalette::Foreground,QColor(Qt::black));
	axisWidget(QwtPlot::xBottom)->setPalette(pal);
	axisWidget(QwtPlot::yLeft)->setPalette(pal);
	axisWidget(QwtPlot::yRight)->setPalette(pal);

	QwtLegend *newLegend = new QwtLegend;
	newLegend->setPalette(pal);
	insertLegend(newLegend);

	//do the printing
    QwtText t = title();
    QwtText oldTitle = title();
    t = QwtText(QString("FTM%1 %2").arg(QTFTM_SPECTROMETER).arg(title().text()));

    setTitle(QString(""));

	if(!ownGraphsBox->isChecked() || plotCurves.size()==1)
        doPrint(xMin,xMax,xWidth,perPage,t.text(),&p);
	else
        doPrint(xMin,xMax,xWidth,perPage,t.text(),&p,true);

    setTitle(oldTitle);

	//now, undo everything
	QwtLegend *l = new QwtLegend;
	l->setDefaultItemMode(QwtLegendData::Checkable);
	l->installEventFilter(this);
	connect(l,&QwtLegend::checked,this,&BatchPlot::toggleCurve);
	insertLegend(l);

	//restore axis colors
	axisWidget(QwtPlot::xBottom)->setPalette(QPalette());
	axisWidget(QwtPlot::yLeft)->setPalette(QPalette());
	axisWidget(QwtPlot::yRight)->setPalette(QPalette());

	//restore scales
	setAxisScale(QwtPlot::xBottom,xScale.first,xScale.second);
	setAxisScale(QwtPlot::yLeft,yScale.first,yScale.second);

	//remove markers and grid (this will delete them all if they're there)
	detachItems(QwtPlotItem::Rtti_PlotGrid);
	detachItems(QwtPlotItem::Rtti_PlotMarker);

	//restore curves
	if(zoneWasVisible)
		zone->setVisible(true);

	for(int i=0; i<plotCurves.size(); i++)
	{
		plotCurves[i]->setPen(pens.at(i));
		plotCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased,false);
		plotCurves[i]->setVisible(true);
	}

	replot();

	QApplication::restoreOverrideCursor();

}

void BatchPlot::printBatch()
{
	//set up options dialog
	QDialog d;
	d.setWindowTitle(QString("Batch Printing Options"));

	//graph-related options
	QGroupBox *graphBox = new QGroupBox(QString("Graph setup"),&d);
	QFormLayout *gbl = new QFormLayout(graphBox);

	QSpinBox *graphsPerPageBox = new QSpinBox(graphBox);
	graphsPerPageBox->setMinimum(1);
    graphsPerPageBox->setMaximum(6);
	graphsPerPageBox->setValue(3);
	graphsPerPageBox->setToolTip(QString("Number of graphs per page"));
	gbl->addRow(QString("Graphs per page"),graphsPerPageBox);

	QSpinBox *scanMinBox = new QSpinBox(graphBox);
	scanMinBox->setToolTip(QString("Starting scan"));
	scanMinBox->setMinimum(metaData.at(0).scanNum);
	scanMinBox->setMaximum(metaData.at(metaData.size()-1).scanNum);
	scanMinBox->setValue(metaData.at(0).scanNum);
	scanMinBox->setSingleStep(1);
	gbl->addRow(QString("First scan"),scanMinBox);

	QSpinBox *scanMaxBox = new QSpinBox(graphBox);
	scanMaxBox->setToolTip(QString("Ending scan"));
	scanMaxBox->setMinimum(metaData.at(0).scanNum);
	scanMaxBox->setMaximum(metaData.at(metaData.size()-1).scanNum);
	scanMaxBox->setValue(metaData.at(metaData.size()-1).scanNum);
	scanMaxBox->setSingleStep(1);
	gbl->addRow(QString("Last scan"),scanMaxBox);

	QSpinBox *scansPerGraphBox = new QSpinBox(graphBox);
	scansPerGraphBox->setToolTip(QString("Number of scans plotted on each graph"));
	scansPerGraphBox->setMinimum(1);
	scansPerGraphBox->setMaximum(8);
	scansPerGraphBox->setValue(8);
	scansPerGraphBox->setSingleStep(1);
	gbl->addRow(QString("Scans per graph"),scansPerGraphBox);

	QDoubleSpinBox *yMinBox = new QDoubleSpinBox(graphBox);
	yMinBox->setToolTip(QString("Minimum value on y axis of each graph"));
    yMinBox->setDecimals(3);
	yMinBox->setMinimum(0.0);
	yMinBox->setMaximum(autoScaleYRange.second*0.9);
	yMinBox->setValue(0.0);
	yMinBox->setSingleStep(autoScaleYRange.second*0.1);
	gbl->addRow(QString("Y min"),yMinBox);

	QDoubleSpinBox *yMaxBox = new QDoubleSpinBox(graphBox);
	yMaxBox->setToolTip(QString("Maximum value on y axis of each graph"));
    yMaxBox->setDecimals(3);
	yMaxBox->setMinimum(0.0);
	yMaxBox->setMaximum(autoScaleYRange.second);
	yMaxBox->setValue(autoScaleYRange.second);
	yMaxBox->setSingleStep(autoScaleYRange.second*0.1);
	gbl->addRow(QString("Y max"),yMaxBox);

	graphBox->setLayout(gbl);

	//batch-related options
	QGroupBox *optionsBox = new QGroupBox("Options",&d);
	QFormLayout *obl = new QFormLayout(optionsBox);

	QCheckBox *labelsCheckBox = new QCheckBox(optionsBox);
	labelsCheckBox->setToolTip(QString("Show labels on graphs"));
	labelsCheckBox->setChecked(true);
	obl->addRow(QString("Show labels"),labelsCheckBox);

	QCheckBox *gridCheckBox = new QCheckBox(optionsBox);
	gridCheckBox->setToolTip(QString("Show a grid on the graph canvas"));
	gridCheckBox->setChecked(true);
	obl->addRow(QString("Grid"),gridCheckBox);

	optionsBox->setLayout(obl);

	QHBoxLayout *dl = new QHBoxLayout();
	dl->addWidget(graphBox);
	dl->addWidget(optionsBox);

	QVBoxLayout *vl = new QVBoxLayout(&d);
	vl->addLayout(dl);

	QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal,&d);
	connect(box->button(QDialogButtonBox::Ok),&QAbstractButton::clicked,&d,&QDialog::accept);
	connect(box->button(QDialogButtonBox::Cancel),&QAbstractButton::clicked,&d,&QDialog::reject);
	vl->addWidget(box);

	d.setLayout(vl);

	int ret = d.exec();
	if(ret != QDialog::Accepted)
		return;

	//launch print dialog
	QPrinter p(QPrinter::HighResolution);
	p.setOrientation(QPrinter::Landscape);
	p.setCreator(QString("QtFTM"));
	QPrintDialog pd(&p);

	if(!pd.exec())
		return;

	//show busy cursor during printing
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//configure plot for printing
	//first, override any invalid settings, and calculate number of pages
	double start = (double)scanMinBox->value()-0.5, end = (double)scanMaxBox->value()+0.5, yMin = yMinBox->value(),
			yMax = yMaxBox->value(), scansPerGraph = scansPerGraphBox->value();
	int perPage = graphsPerPageBox->value();

	if(start == end)
	{
		start = (double)metaData.at(0).scanNum-0.5;
		end = (double)metaData.at(metaData.size()-1).scanNum+0.5;
	}
	if(start > end)
		qSwap(start,end);

	if(yMin == yMax)
	{
		yMin = autoScaleYRange.first;
		yMax = autoScaleYRange.second;
	}
	if(yMin > yMax)
		qSwap(yMin,yMax);

	//store pen
	QPen curvePen =  plotCurves.at(0)->pen();

	//store text color
	QColor textColor = plotMarkers.at(0)->label().color();

	plotCurves[0]->setPen(QPen(Qt::black,1.0));
	plotCurves[0]->setRenderHint(QwtPlotItem::RenderAntialiased);

	//grid
	if(gridCheckBox->isChecked())
	{
		QwtPlotGrid *pGrid = new QwtPlotGrid;
		pGrid->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
		pGrid->setMajorPen(QPen(QBrush(Qt::gray),1.0,Qt::DashLine));
		pGrid->setMinorPen(QPen(QBrush(Qt::lightGray),1.0,Qt::DotLine));
		pGrid->enableXMin(true);
		pGrid->enableYMin(true);
		pGrid->setRenderHint(QwtPlotItem::RenderAntialiased);
		pGrid->attach(this);
	}

	for(int i=0; i<plotMarkers.size(); i++)
	{
		plotMarkers[i]->setVisible(true);
		QwtText t = plotMarkers.at(i)->label();
		t.setBackgroundBrush(QBrush(Qt::white));
		t.setColor(Qt::black);
		plotMarkers[i]->setLabel(t);
		plotMarkers[i]->setRenderHint(QwtPlotItem::RenderAntialiased,true);
	}

	//set y range
	//store plot ranges
	QPair<double,double> xScale;
    xScale.first = axisScaleDiv(QwtPlot::xBottom).lowerBound();
	xScale.second = axisScaleDiv(QwtPlot::xBottom).upperBound();

	QPair<double,double> yScale;
	yScale.first = axisScaleDiv(QwtPlot::yLeft).lowerBound();
	yScale.second = axisScaleDiv(QwtPlot::yLeft).upperBound();
	setAxisScale(QwtPlot::yLeft,yMin,yMax);

	//un-filter data
	plotCurves[0]->setSamples(plotCurveData.at(0));

	//hide zone
	bool zoneWasVisible = zone->isVisible();
	if(zoneWasVisible)
		zone->setVisible(false);

	//recolor axes
	QPalette pal;
	pal.setColor(QPalette::Text,QColor(Qt::black));
	pal.setColor(QPalette::Foreground,QColor(Qt::black));
	axisWidget(QwtPlot::xBottom)->setPalette(pal);
	axisWidget(QwtPlot::yLeft)->setPalette(pal);
	axisWidget(QwtPlot::yRight)->setPalette(pal);

	//do the printing
    QwtText t = title();
    QwtText oldTitle = title();
    t = QwtText(QString("FTM%1 %2").arg(QTFTM_SPECTROMETER).arg(title().text()));

    setTitle(QString(""));
    doPrint(start,end,(double)scansPerGraph,perPage,t.text(),&p);

	//now, undo everything
    setTitle(oldTitle);

	//restore axis colors
	axisWidget(QwtPlot::xBottom)->setPalette(QPalette());
	axisWidget(QwtPlot::yLeft)->setPalette(QPalette());
	axisWidget(QwtPlot::yRight)->setPalette(QPalette());

	//restore scales
	setAxisScale(QwtPlot::xBottom,xScale.first,xScale.second);
	setAxisScale(QwtPlot::yLeft,yScale.first,yScale.second);

	//remove grid (this will delete it if it's there)
	detachItems(QwtPlotItem::Rtti_PlotGrid);

	//restore curves
	if(zoneWasVisible)
		zone->setVisible(true);

	plotCurves[0]->setPen(curvePen);
	plotCurves[0]->setRenderHint(QwtPlotItem::RenderAntialiased,false);

	for(int i=0; i<plotMarkers.size(); i++)
	{
		QwtText t = plotMarkers.at(i)->label();
		t.setBackgroundBrush(QBrush());
		t.setColor(textColor);
		plotMarkers[i]->setLabel(t);
		plotMarkers[i]->setRenderHint(QwtPlotItem::RenderAntialiased,false);
	}

	replot();

	QApplication::restoreOverrideCursor();

}

void BatchPlot::printAttn()
{
    //no options for this. Just put one curve on each page, in black, with a grid. No scan numbers, always autoscaling
    //launch print dialog
    QPrinter p(QPrinter::HighResolution);
    p.setOrientation(QPrinter::Landscape);
    p.setCreator(QString("QtFTM"));
    QPrintDialog pd(&p);

    if(!pd.exec())
        return;

    //show busy cursor during printing
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    //store and set pens
    QList<QPen> pens;
    for(int i=0; i<plotCurves.size(); i++)
    {
        pens.append(plotCurves.at(i)->pen());
        plotCurves[i]->setPen(Qt::black,1.0);
        plotCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased,true);
        plotCurves[i]->setSamples(plotCurveData.at(i));
    }

    //add grid
    QwtPlotGrid *pGrid = new QwtPlotGrid;
    pGrid->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
    pGrid->setMajorPen(QPen(QBrush(Qt::gray),1.0,Qt::DashLine));
    pGrid->setMinorPen(QPen(QBrush(Qt::lightGray),1.0,Qt::DotLine));
    pGrid->enableXMin(true);
    pGrid->enableYMin(true);
    pGrid->setRenderHint(QwtPlotItem::RenderAntialiased);
    pGrid->attach(this);

    //store plot ranges
    QPair<double,double> xScale;
    xScale.first = axisScaleDiv(QwtPlot::xBottom).lowerBound();
    xScale.second = axisScaleDiv(QwtPlot::xBottom).upperBound();

    QPair<double,double> yScale;
    yScale.first = axisScaleDiv(QwtPlot::yLeft).lowerBound();
    yScale.second = axisScaleDiv(QwtPlot::yLeft).upperBound();

    //recolor axes
    QPalette pal;
    pal.setColor(QPalette::Text,QColor(Qt::black));
    pal.setColor(QPalette::Foreground,QColor(Qt::black));
    axisWidget(QwtPlot::xBottom)->setPalette(pal);
    axisWidget(QwtPlot::yLeft)->setPalette(pal);
    axisWidget(QwtPlot::yRight)->setPalette(pal);

    QwtLegend *newLegend = new QwtLegend;
    newLegend->setPalette(pal);
    insertLegend(newLegend);

    //do printing
    QwtText t = title();
    QwtText oldTitle = title();
    t = QwtText(QString("FTM%1 %2").arg(QTFTM_SPECTROMETER).arg(title().text()));

    setTitle(QString(""));
    double xMin = plotCurveData.at(0).at(0).x();
    double xMax = plotCurveData.at(0).at(plotCurveData.at(0).size()-1).x();
    doPrint(xMin,xMax,xMax-xMin,1,t.text(),&p,true,true);

    //now, undo everything
    setTitle(oldTitle);

    QwtLegend *l = new QwtLegend;
    l->setDefaultItemMode(QwtLegendData::Checkable);
    l->installEventFilter(this);
    connect(l,&QwtLegend::checked,this,&BatchPlot::toggleCurve);
    insertLegend(l);

    //restore axis colors
    axisWidget(QwtPlot::xBottom)->setPalette(QPalette());
    axisWidget(QwtPlot::yLeft)->setPalette(QPalette());
    axisWidget(QwtPlot::yRight)->setPalette(QPalette());

    //restore scales
    setAxisScale(QwtPlot::xBottom,xScale.first,xScale.second);
    setAxisScale(QwtPlot::yLeft,yScale.first,yScale.second);

    //remove markers and grid (this will delete them all if they're there)
    detachItems(QwtPlotItem::Rtti_PlotGrid);
    detachItems(QwtPlotItem::Rtti_PlotMarker);

    for(int i=0; i<plotCurves.size(); i++)
    {
        plotCurves[i]->setPen(pens.at(i));
        plotCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased,false);
        plotCurves[i]->setVisible(true);
    }

    replot();

    QApplication::restoreOverrideCursor();


}

void BatchPlot::doPrint(double start, double end, double xRange, int plotsPerPage, QString title, QPrinter *pr, bool oneCurvePerPlot, bool autoYRanges)
{
	int numPages = (int)ceil((end-start)/xRange/(double)plotsPerPage);
    if(oneCurvePerPlot)
	{
		int numRanges = plotCurves.size();
		numPages = (int)ceil((end-start)*numRanges/xRange/(double)plotsPerPage);
	}
	QwtPlotRenderer rend;
	rend.setDiscardFlags(QwtPlotRenderer::DiscardBackground | QwtPlotRenderer::DiscardCanvasBackground);

	QFrame df(this);
	df.setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	QPalette pal;
	pal.setColor(QPalette::Base,QColor(Qt::white));
	pal.setColor(QPalette::AlternateBase,QColor(Qt::white));
	pal.setColor(QPalette::Text,QColor(Qt::black));
	pal.setColor(QPalette::Window,QColor(Qt::white));
	pal.setColor(QPalette::WindowText,QColor(Qt::black));
	df.setPalette(pal);

	QFont theFont(QString("sans-serif"),8);

	QLabel titleLabel(title);
	titleLabel.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	titleLabel.setFont(QFont(theFont,pr));

	QLabel pageLabel;
	pageLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	pageLabel.setFont(QFont(theFont,pr));

	QHBoxLayout hl;
	hl.setContentsMargins(0,0,0,0);
	hl.addWidget(&titleLabel,1,Qt::AlignLeft);
	hl.addWidget(&pageLabel,0,Qt::AlignRight);
	df.setLayout(&hl);
	df.setContentsMargins(0,0,0,0);

	//compute height of title layout
	QFontMetrics fm(theFont,pr);
	QFontMetrics fm2(theFont);
	int titleHeight = fm.height();
	double scale = (double)fm.height()/(double)fm2.height();
	df.setGeometry(0,0,(double)pr->pageRect().width()/scale,(double)fm2.height());

	QList<QRect> graphRects;
	for(int i=0; i<plotsPerPage; i++)
	{
		QRect r;
		r.setWidth(pr->pageRect().width());
		r.setHeight((pr->pageRect().height()-titleHeight)/plotsPerPage);
		r.moveTop(i*r.height() + titleHeight);
		graphRects.append(r);
	}

    int curveIndex = 0, curveCount = 0;
	QwtLegend *leg = static_cast<QwtLegend*>(legend());

	QPainter p;
	p.begin(pr);

	//all preparation is complete, enter render loop
	for(int page = 0; page<numPages; page++)
	{
		if(page>0)
			pr->newPage();

		//set page number in title
		pageLabel.setText(QString("Page %1/%2").arg(page+1).arg(numPages));
		p.scale(scale,scale);

		//render title bar
		df.render(&p,QPoint(),QRegion(),DrawChildren);

		p.scale(1.0/scale,1.0/scale);


        if(!oneCurvePerPlot)
		{
			//loop over graph rectangles
			for(int rect=0; rect<graphRects.size(); rect++)
			{
				double xMin = start + (double)((page*plotsPerPage) + rect)*xRange;
				double xMax = start + (double)((page*plotsPerPage) + rect + 1)*xRange;
				setAxisScale(QwtPlot::xBottom,xMin,xMax);
				QwtPlot::replot();
				rend.render(this,&p,graphRects.at(rect));

				if(xMax >= end)
					break;
			}
		}
		else
		{
			//only one range plotted on each graph, so we need keep cycling through those as well
			for(int rect=0; rect<graphRects.size(); rect++)
			{
                double xMin = start + (double)curveCount*xRange;
                double xMax = start + (double)(curveCount+1)*xRange;

                //only rescale axis when curveIndex cycles back to 0
                if(curveIndex==0)
					setAxisScale(QwtPlot::xBottom,xMin,xMax);

				//set the appropriate curve to be visible
				for(int j=0;j<plotCurves.size();j++)
				{
                    if(j==curveIndex)
					{
						plotCurves[j]->setVisible(true);
						static_cast<QwtLegendLabel*>(leg->legendWidget(itemToInfo(plotCurves[j])))->show();
                        if(autoYRanges)
                            setAxisScale(QwtPlot::yLeft,yMinMaxes.at(j).first,yMinMaxes.at(j).second);
					}
					else
					{
						plotCurves[j]->setVisible(false);
						static_cast<QwtLegendLabel*>(leg->legendWidget(itemToInfo(plotCurves[j])))->hide();
					}
				}
				QwtPlot::replot();
				rend.render(this,&p,graphRects.at(rect));

				//break out of the plotting loop if we're done
                if(xMax >= end && curveIndex==plotCurves.size())
					break;

                //increment curveIndex. if we've done the last one, reset it to 0 and increment the curve count
                curveIndex = (curveIndex+1) % plotCurves.size();
                if(curveIndex==0)
                    curveCount++;

			}
		}
	}

	p.end();
}
