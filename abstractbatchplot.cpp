#include "abstractbatchplot.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include <qwt6/qwt_plot_renderer.h>
#include <qwt6/qwt_legend_label.h>
#include <qwt6/qwt_legend.h>
#include <qwt6/qwt_picker_machine.h>
#include <qwt6/qwt_symbol.h>
#include <qwt6/qwt_scale_map.h>

AbstractBatchPlot::AbstractBatchPlot(QString name, QWidget *parent) :
    ZoomPanPlot(name,parent), d_zoneScanNum(0), d_showZonePending(false), d_recalcZoneOnResize(false), d_doNotReplot(false), d_hideBadZones(false), d_hidePlotLabels(false)
{
    QFont labelFont(QString("sans serif"),8);
    setAxisFont(QwtPlot::xBottom,labelFont);
    setAxisFont(QwtPlot::yLeft,labelFont);
    setAxisFont(QwtPlot::yRight,labelFont);

    QwtPlotPicker *picker = new QwtPlotPicker(this->canvas());
    picker->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
    picker->setStateMachine(new QwtPickerClickPointMachine);
    picker->setMousePattern(QwtEventPattern::MouseSelect1,Qt::RightButton);
    picker->setTrackerMode(QwtPicker::AlwaysOn);
    picker->setTrackerPen(QPen(QPalette().color(QPalette::Text)));
    picker->setEnabled(true);

    p_selectedZone = new QwtPlotZoneItem();
    p_selectedZone = new QwtPlotZoneItem();
    p_selectedZone->setOrientation(Qt::Vertical);
    QColor zoneColor = QPalette().color(QPalette::AlternateBase);
    zoneColor.setAlpha(128);
    p_selectedZone->setBrush(QBrush(zoneColor));
    p_selectedZone->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
    p_selectedZone->setInterval(0.0,0.0);
    p_selectedZone->setVisible(false);
    p_selectedZone->attach(this);

    p_calCurve = nullptr;

    connect(this,&AbstractBatchPlot::plotRightClicked,this,&AbstractBatchPlot::launchContextMenu);
}

AbstractBatchPlot::~AbstractBatchPlot()
{
    detachItems(QwtPlotItem::Rtti_PlotItem);
}

void AbstractBatchPlot::launchContextMenu(QPoint pos)
{
    QMenu *m = contextMenu();

    double x = canvasMap(QwtPlot::xBottom).invTransform(canvas()->mapFromGlobal(pos).x());

    if(!d_metaDataList.isEmpty() && d_metaDataList.first().type != QtFTM::Attenuation)
    {
        bool hasCal = false;
        bool hasScan = false;
        for(int i=0; i<d_metaDataList.size(); i++)
        {
            if(d_metaDataList.at(i).isCal)
                hasCal = true;
            else
                hasScan = true;

            if(hasCal && hasScan)
                break;
        }

        QAction *viewScanAction = m->addAction(QString("View nearest scan"));
        if(!hasScan)
            viewScanAction->setEnabled(false);
        connect(viewScanAction,&QAction::triggered,[=](){ loadScan(x); });

        QAction *viewCalAction = m->addAction(QString("View nearest cal scan"));
        if(!hasCal)
            viewCalAction->setEnabled(false);
        connect(viewCalAction,&QAction::triggered,[=](){ loadCalScan(x); });
    }

    m->popup(pos);
}

void AbstractBatchPlot::loadScan(const double x)
{
    if(d_metaDataList.isEmpty())
        return;

    int firstScanIndex = -1, lastScanIndex = -1;
    for(int i=0; i<d_metaDataList.size(); i++)
    {
        if(!d_metaDataList.at(i).isCal)
        {
            firstScanIndex = i;
            break;
        }
    }
    for(int i = d_metaDataList.size()-1; i>=0; i--)
    {
        if(!d_metaDataList.at(i).isCal)
        {
            lastScanIndex = i;
            break;
        }
    }

    if(firstScanIndex < 0 || lastScanIndex < 0)
	    return;

    bool scanningDown = false;
    //if this is a survey or DR measurement, it's possible to scan down in frequency
    //it if there's only 1 cal scan so far, it doesn't matter (this is only used to order multiple scans)
    if(d_metaDataList.at(firstScanIndex).minXVal > d_metaDataList.at(lastScanIndex).minXVal)
        scanningDown = true;

    //make a list of non-cal scans in increasing frequency
    QList<QtFTM::BatchPlotMetaData> scans;
    for(int i=firstScanIndex; i<=lastScanIndex; i++)
    {
        if(!d_metaDataList.at(i).isCal)
        {
            if(scanningDown)
                scans.prepend(d_metaDataList.at(i));
            else
                scans.append(d_metaDataList.at(i));
        }
    }

    //locate scan number of nearest non-cal scan
    if(x<scans.first().minXVal)
    {
        emit requestScan(scans.at(0).scanNum);
        return;
    }
    else if(x>scans.last().maxXVal)
    {
        emit requestScan(scans.last().scanNum);
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

void AbstractBatchPlot::loadCalScan(const double x)
{
    if(d_metaDataList.isEmpty())
        return;

    int firstCalIndex = -1, lastCalIndex = -1;
    for(int i=0; i<d_metaDataList.size(); i++)
    {
        if(d_metaDataList.at(i).isCal)
        {
            firstCalIndex = i;
            break;
        }
    }
    for(int i = d_metaDataList.size()-1; i>=0; i--)
    {
        if(d_metaDataList.at(i).isCal)
        {
            lastCalIndex = i;
            break;
        }
    }

    if(firstCalIndex < 0 || lastCalIndex < 0)
	    return;

    bool scanningDown = false;
    //if this is a survey or DR measurement, it's possible to scan down in frequency
    //it if there's only 1 cal scan so far, it doesn't matter (this is only used to order multiple scans)
    if(d_metaDataList.at(firstCalIndex).minXVal > d_metaDataList.at(lastCalIndex).minXVal)
        scanningDown = true;

    //make a list of cal scans in increasing frequency
    QList<QtFTM::BatchPlotMetaData> calScans;
    for(int i=firstCalIndex; i<=lastCalIndex; i++)
    {
        if(d_metaDataList.at(i).isCal)
        {
            if(scanningDown)
                calScans.prepend(d_metaDataList.at(i));
            else
                calScans.append(d_metaDataList.at(i));
        }
    }

    //locate number of nearest cal scan
    if(calScans.size() == 1)
    {
        emit requestScan(calScans.first().scanNum);
        return;
    }
    else if(x<calScans.first().minXVal || qAbs(x - calScans.first().minXVal) < qAbs(x - calScans.at(1).minXVal))
    {
	   emit requestScan(calScans.first().scanNum);
        return;
    }
    else if(x>calScans.last().maxXVal)
    {
        emit requestScan(calScans.last().scanNum);
        return;
    }
    for(int i=1; i<calScans.size()-1; i++)
    {
	    double before = qAbs(x - calScans.at(i-1).minXVal);
	    double thisd = qAbs(x - calScans.at(i).minXVal);
        double after = qAbs(x - calScans.at(i+1).minXVal);
	   if(thisd <= before && thisd <= after)
        {
            emit requestScan(calScans.at(i).scanNum);
            return;
        }
    }

    //if we've made it here, then it must be the last scan in the list
    emit requestScan(calScans.last().scanNum);
}

void AbstractBatchPlot::setSelectedZone(int scanNum)
{
    d_zoneScanNum = scanNum;
    if(d_metaDataList.isEmpty())
    {
        if(p_selectedZone->plot() == this)
        {
            p_selectedZone->setVisible(false);
            replot();
        }
        d_showZonePending = true;
        return;
    }

    if(d_metaDataList.at(0).scanNum < 0)
    {
        if(p_selectedZone->plot() == this)
        {
            p_selectedZone->setVisible(false);
            replot();
        }
        d_showZonePending = false;
        return;
    }

    if(scanNum < d_metaDataList.first().scanNum || scanNum > d_metaDataList.last().scanNum)
    {
        if(p_selectedZone->plot() == this)
        {
            p_selectedZone->setVisible(false);
            replot();
        }

        //when a scan is complete, the analysis widget gets updated BEFORE the batch manager sends out the new data
        //that means this function gets called before the scan has been put into the metadata array
        //so, if the scan number is 1 greater than the most recent entry, we'll want to draw the zone as soon as the data come in
        if(scanNum >= d_metaDataList.first().scanNum + d_metaDataList.size())
            d_showZonePending = true;

        return;

    }

    int scanIndex = scanNum - d_metaDataList.first().scanNum;
    formatSelectedZone(scanIndex);
    p_selectedZone->setVisible(true);
    replot();

}

void AbstractBatchPlot::formatSelectedZone(int metadataIndex)
{
    if(metadataIndex < 0 || metadataIndex >= d_metaDataList.size())
        return;

    QtFTM::BatchPlotMetaData md = d_metaDataList.at(metadataIndex);
    setZoneWidth(p_selectedZone,md);
    if(md.isCal && p_calCurve != nullptr)
        d_recalcZoneOnResize = true;
    else
        d_recalcZoneOnResize = false;

    if(md.isCal)
        p_selectedZone->setPen(QPen(QColor(QPalette().color(QPalette::Text)),1.0,Qt::DotLine));
    else
        p_selectedZone->setPen(QPen(QColor(QPalette().color(QPalette::Text))));
}

void AbstractBatchPlot::setZoneWidth(QwtPlotZoneItem *zone, QtFTM::BatchPlotMetaData md)
{
    if(zone == nullptr)
        return;

    if(md.isCal)
    {
        if(p_calCurve != nullptr)
        {
            double symbolHalfWidth = p_calCurve->symbol()->size().width()/2.0;
            QwtScaleMap map = canvasMap(QwtPlot::xBottom);

            //to calculate the width, we have to transform the marker position into its pixel position,
            //add the width of the marker, and transform it back to plot coordinates
            //this isn't perfect, because of rounding pixel positions, so an extra pixel is added on each side
            //to make sure the whole marker gets enclosed
            double zoneMin = map.invTransform(map.transform(md.minXVal)-symbolHalfWidth-1.0);
            double zoneMax = map.invTransform(map.transform(md.minXVal)+symbolHalfWidth+1.0);

            zone->setInterval(zoneMin,zoneMax);
        }
        else
            zone->setInterval(md.minXVal,md.maxXVal);
    }
    else
        zone->setInterval(md.minXVal,md.maxXVal);
}

void AbstractBatchPlot::filterData()
{
    if(d_plotCurveData.isEmpty())
        return;

    if(!d_plotMarkers.isEmpty() && !d_hidePlotLabels)
    {
	   //loop over labels, seeing if the next will overlap
	   int activeIndex = 0;

	   QFontMetrics fm(d_plotMarkers.at(0)->label().font());
	   double min = axisScaleDiv(QwtPlot::xBottom).lowerBound();
	   double max = axisScaleDiv(QwtPlot::xBottom).upperBound();


	   for(int i=0; i<d_plotMarkers.size(); i++)
	   {
		  auto thisRange = calculateMarkerBoundaries(fm,i);
		  if(thisRange.first < min)
			 d_plotMarkers[i]->setVisible(false);
		  else
		  {
			 activeIndex = i;
			 d_plotMarkers[i]->setVisible(true);
			 break;
		  }
	   }

	   auto activeRange = calculateMarkerBoundaries(fm,activeIndex);
	   for(int i=activeIndex+1; i<d_plotMarkers.size(); i++)
	   {
		  //calculate right edge of active marker, and left edge of current marker
		  //get widest text line
		  auto thisRange = calculateMarkerBoundaries(fm,i);

		  //hide this label if it overlaps with the active label
		  if(thisRange.first < activeRange.second || thisRange.second > max)
			 d_plotMarkers[i]->setVisible(false);
		  else //otherwise, it is visible, and will be the reference for the next iteration of the loop
		  {
			 activeRange = thisRange;
			 d_plotMarkers[i]->setVisible(true);
			 activeIndex = i;
		  }

	   }

    }


    for(int i=0; i<d_plotCurveData.size(); i++)
    {

        QVector<QPointF> d = d_plotCurveData.at(i);
        if(d.isEmpty())
            continue;

        if(d.size() < 2*canvas()->width())
        {
            d_plotCurves[i]->setSamples(d);
            continue;
        }

        int firstPixel = 0;
        int lastPixel = canvas()->width();
        int pixelIncr = 1;
        QwtScaleMap map = canvasMap(QwtPlot::xBottom);

        if(d.last().x() < d.first().x())
        {
            qSwap(firstPixel,lastPixel);
            pixelIncr = -1;
        }

        QVector<QPointF> filtered;
        filtered.reserve(2*canvas()->width() + 2);

        //find first data point that is in the range of the plot
        int dataIndex = 0;
        while(dataIndex+1 < d.size())
        {
            if(pixelIncr > 0)
            {
                if(map.transform(d.at(dataIndex).x()) > static_cast<double>(firstPixel))
                    break;
            }
            else if(map.transform(d.at(dataIndex).x()) < static_cast<double>(firstPixel))
                break;

            dataIndex++;
        }

        //add previous point to filtered array
        //this ensures curve goes to edge of plot
        if(dataIndex - 1 >= 0)
            filtered.append(d.at(dataIndex-1));

        //dataIndex is the first point in range of the plot. loop over pixels, compressing data
        for(int p = firstPixel; p != lastPixel; p+=pixelIncr)
        {
            double pixel = static_cast<double>(p);
            double min = d.at(dataIndex).y();
            double max = min;
            int numPnts = 0;
            int minIndex = dataIndex;
            int maxIndex = dataIndex;

            while(dataIndex+1 < d.size())
            {
                if(pixelIncr > 0)
                {
                    if(map.transform(d.at(dataIndex).x()) > pixel+1.0)
                        break;
                }
                else if(map.transform(d.at(dataIndex).x()) < pixel-1.0)
                    break;

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
            else if(numPnts > 1)
            {
                QPointF first(map.invTransform(pixel),d.at(minIndex).y());
                QPointF second(map.invTransform(pixel),d.at(maxIndex).y());
                filtered.append(first);
                filtered.append(second);
            }
        }

        if(dataIndex < d.size())
            filtered.append(d.at(dataIndex));

        d_plotCurves[i]->setSamples(filtered);
    }
}

void AbstractBatchPlot::exportXY()
{
    if(d_plotCurveData.isEmpty())
        return;

    QString labelBase = title().text().toLower();
    labelBase.replace(QString(" "),QString(""));
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString path = s.value(QString("exportPath"),QDir::homePath()).toString();
    QString fileName = QFileDialog::getSaveFileName(this,QString("XY Export"),path + QString("/%1.txt").arg(labelBase));

    if(fileName.isEmpty())
        return;

    QFile f(fileName);
    if(!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this,QString("Export Failed"),QString("Could not open selected file (%1) for writing. Please try again with a different filename.").arg(f.fileName()));
        return;
    }

    QString newPath = QFileInfo(fileName).dir().absolutePath();
    s.setValue(QString("exportPath"),newPath);

    QTextStream t(&f);
    t.setRealNumberNotation(QTextStream::ScientificNotation);
    t.setRealNumberPrecision(12);
    QString tab("\t");
    QString nl("\n");

    t << QString("x%1").arg(labelBase);
    for(int i=0;i<d_plotCurveData.size();i++)
    {
        if(d_plotCurveMetaData.at(i).visible)
            t << tab << QString("y%1%2").arg(i).arg(labelBase);
    }

    for(int i=0; i<d_plotCurveData.at(0).size(); i++)
    {
        t << nl << d_plotCurveData.at(0).at(i).x();
        for(int j=0; j<d_plotCurveData.size(); j++)
        {
            if(d_plotCurveMetaData.at(j).visible)
            {
                if(i < d_plotCurveData.at(j).size())
                    t << tab << d_plotCurveData.at(j).at(i).y();
                else
                    t << tab << 0.0;
            }
        }
    }

    if(!d_calCurveData.isEmpty())
    {
        t << nl << nl << QString("calx%1").arg(labelBase) << tab << QString("caly%1").arg(labelBase);
        for(int i=0;i<d_calCurveData.size();i++)
            t << nl << d_calCurveData.at(i).x() << tab << d_calCurveData.at(i).y();
    }

    t.flush();
    f.close();
}

void AbstractBatchPlot::toggleCurve(QVariant item, bool hide, int index)
{
    if(d_plotCurveMetaData.isEmpty())
        return;

    Q_UNUSED(index)
    QwtPlotCurve *c = static_cast<QwtPlotCurve*>(infoToItem(item));
    if(c)
    {
        //find curve in metadata list to update visibility
        for(int i=0; i<d_plotCurveMetaData.size(); i++)
        {
            if(c == d_plotCurveMetaData.at(i).curve)
                d_plotCurveMetaData[i].visible = !hide;
        }
        c->setVisible(!hide);

        //recalculate y range
        int index = 0;
        while(index < d_plotCurveMetaData.size() && !d_plotCurveMetaData.at(index).visible)
            index++;

        if(index < d_plotCurveMetaData.size())
        {
            double min = d_plotCurveMetaData.at(index).yMin;
            double max = d_plotCurveMetaData.at(index).yMax;


            for(int i=index+1; i<d_plotCurveMetaData.size();i++)
            {
                if(d_plotCurveMetaData.at(i).visible)
                {
                    min = qMin(d_plotCurveMetaData.at(i).yMin,min);
                    max = qMax(d_plotCurveMetaData.at(i).yMax,max);
                }
            }

            setAxisAutoScaleRange(QwtPlot::yLeft,0.0,max);
        }

        replot();
    }
}

void AbstractBatchPlot::toggleHideBadZones(bool hide)
{
    d_hideBadZones = hide;
    for(int i=0; i<d_badTuneZones.size(); i++)
        d_badTuneZones[i].zone->setVisible(!hide);

    replot();
}

void AbstractBatchPlot::togglePlotLabels(bool on)
{
    d_hidePlotLabels = on;

    for(int i=0; i<d_plotMarkers.size(); i++)
	   d_plotMarkers[i]->setVisible(!d_hidePlotLabels);

    replot();
}

void AbstractBatchPlot::addBadZone(QtFTM::BatchPlotMetaData md)
{
    if(!md.badTune)
        return;

    QwtPlotZoneItem *zone = new QwtPlotZoneItem();
    zone->setOrientation(Qt::Vertical);
    QColor c(Qt::red);
    c.setAlpha(128);
    zone->setBrush(QBrush(c));
    zone->setZ(10.0);
    c.setAlpha(0);
    zone->setPen(QPen(c));
    zone->setVisible(!d_hideBadZones);

    bool resize = false;
    if(md.isCal && p_calCurve != nullptr)
        resize = true;

    setZoneWidth(zone,md);
    zone->attach(this);

    BadZone bz;
    bz.zone = zone;
    bz.md = md;
    bz.recalcWidthOnResize = resize;

    d_badTuneZones.append(bz);

}

QMenu *AbstractBatchPlot::contextMenu()
{
    QMenu *out = ZoomPanPlot::contextMenu();

    if(!d_metaDataList.isEmpty())
    {
        QAction *exportAction = out->addAction(QString("Export XY..."));
        connect(exportAction,&QAction::triggered,this,&AbstractBatchPlot::exportXY);
    }

    QAction *hideAction = out->addAction(QString("Hide bad tune zones"));
    hideAction->setCheckable(true);
    hideAction->setChecked(d_hideBadZones);
    connect(hideAction,&QAction::toggled,this,&AbstractBatchPlot::toggleHideBadZones);

    if(!d_plotMarkers.isEmpty())
    {
	    QAction *hideLabelsAction = out->addAction(QString("Hide labels"));
	    hideLabelsAction->setCheckable(true);
	    hideLabelsAction->setChecked(d_hidePlotLabels);
	    connect(hideLabelsAction,&QAction::triggered,[=](){ togglePlotLabels(!d_hidePlotLabels); });
    }

    return out;
}

bool AbstractBatchPlot::eventFilter(QObject *obj, QEvent *ev)
{
    return ZoomPanPlot::eventFilter(obj,ev);
}

void AbstractBatchPlot::replot()
{
    if(!d_doNotReplot)
    {
	    if(!d_plotMarkers.isEmpty())
	    {
		    if(d_hidePlotLabels)
		    {
			    //get max height for left and right axes
			    double lmax = 0.0;
			    for(int i=0; i<d_plotCurveMetaData.size(); i++)
				    lmax = qMax(d_plotCurveMetaData.at(i).yMax,lmax);
			    double rMax = 0.0;
			    for(int i=0; i<d_calCurveData.size(); i++)
				    rMax = qMax(d_calCurveData.at(i).y(),rMax);

			    setAxisAutoScaleRange(QwtPlot::yLeft,0.0,lmax);
			    setAxisAutoScaleRange(QwtPlot::yRight,0.0,rMax);
		    }
		    else
		    {
			    setAxisAutoScaleRange(QwtPlot::yLeft,0.0,calculateAxisMaxWithLabel(QwtPlot::yLeft));
			    setAxisAutoScaleRange(QwtPlot::yRight,0.0,calculateAxisMaxWithLabel(QwtPlot::yRight));
		    }
	    }

        ZoomPanPlot::replot();
        bool replotAgain = false;
        if(d_recalcZoneOnResize && !d_metaDataList.isEmpty())
        {
            int index = d_zoneScanNum - d_metaDataList.first().scanNum;
            if(index >=0 && index < d_metaDataList.size())
            {
                formatSelectedZone(index);
                replotAgain = true;
            }
        }
        if(!d_badTuneZones.isEmpty() && !d_hideBadZones)
        {
            for(int i=0; i<d_badTuneZones.size(); i++)
            {
                if(d_badTuneZones.at(i).recalcWidthOnResize)
                {
                    setZoneWidth(d_badTuneZones.at(i).zone,d_badTuneZones.at(i).md);
                    replotAgain = true;
                }
            }
        }

        if(replotAgain)
            ZoomPanPlot::replot();

    }
}

double AbstractBatchPlot::calculateAxisMaxWithLabel(QwtPlot::Axis axis) const
{
	QPair<double,double> yRange = qMakePair(0.0,0.0);
	if(axis == QwtPlot::yLeft)
	{
	    for(int i=0; i <d_plotCurveMetaData.size();i++)
		   yRange.second = qMax(yRange.second,d_plotCurveMetaData.at(i).yMax);
	}
	else
	{
	    for(int i=0; i<d_calCurveData.size(); i++)
		   yRange.second = qMax(yRange.second,d_calCurveData.at(i).y());
	}

	if(d_plotMarkers.isEmpty())
		return yRange.second;

	int height = 0;
	QFontMetrics fm(d_plotMarkers.at(0)->label().font());
	for(int i=0;i<d_plotMarkers.size();i++)
	{
		if((axis == QwtPlot::yLeft && !d_metaDataList.at(i).isCal) ||
				(axis == QwtPlot::yRight && d_metaDataList.at(i).isCal))
		{
			QString text = d_plotMarkers.at(i)->label().text();
			int numLines = text.split(QString("\n"),QString::SkipEmptyParts).size();
			height = qMax(height,fm.boundingRect(text).height()*numLines);
		}
	}

	double scaling = (yRange.second - yRange.first)/(double)canvas()->height();
	return static_cast<double>(height+20)*scaling + yRange.second;

}

QPair<double, double> AbstractBatchPlot::calculateMarkerBoundaries(QFontMetrics fm, int index)
{
    if(index < 0 || index >= d_plotMarkers.size())
	   return qMakePair(-1.0,-1.0);

    double scaleMin = axisScaleDiv(QwtPlot::xBottom).lowerBound();
    double scaleMax = axisScaleDiv(QwtPlot::xBottom).upperBound();
    double scaling = (scaleMax-scaleMin)/static_cast<double>(canvas()->width());

    QStringList lines = d_plotMarkers.at(index)->label().text().split(QString("\n"),QString::SkipEmptyParts);
    int w = 0;
    for(int j=0; j<lines.size(); j++)
	   w = qMax(w,fm.boundingRect(lines.at(j)).width())+5;

    double min = (double)d_metaDataList.at(index).scanNum - (double)w*scaling/2.0;
    double max = (double)d_metaDataList.at(index).scanNum + (double)w*scaling/2.0;

    return qMakePair(min,max);
}

void AbstractBatchPlot::doPrint(double start, double end, double xRange, int plotsPerPage, QString title, QPrinter *pr, bool oneCurvePerPlot, bool autoYRanges)
{
    int numPages = (int)ceil((end-start)/xRange/(double)plotsPerPage);
    if(oneCurvePerPlot)
    {
        int numRanges = d_plotCurves.size();
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

    if(!d_plotMarkers.isEmpty() && d_plotMarkers.first()->isVisible())
    {
	    QFontMetrics fm3(d_plotMarkers.at(0)->label().font(),pr);
	    int lheight = 0, rheight = 0;
	    for(int i=0;i<d_plotMarkers.size();i++)
	    {
		    if(!d_metaDataList.at(i).isCal)
		    {
			    QString text = d_plotMarkers.at(i)->label().text();
			    int numLines = text.split(QString("\n"),QString::SkipEmptyParts).size();
			    lheight = qMax(lheight,fm3.boundingRect(text).height()*numLines);
		    }
		    else
		    {
			    QString text = d_plotMarkers.at(i)->label().text();
			    int numLines = text.split(QString("\n"),QString::SkipEmptyParts).size();
			    rheight = qMax(rheight,fm3.boundingRect(text).height()*numLines);
		    }
	    }

	    QwtPlot::replot();
	    auto ldiv = axisScaleDiv(QwtPlot::yLeft);
	    auto rdiv = axisScaleDiv(QwtPlot::yRight);

	    //x axis takes up ~1000 pts (printer scale)
	    double lscaling = (ldiv.upperBound()-ldiv.lowerBound())/((double)graphRects.first().height()-1000);
	    double lyMax = static_cast<double>(lheight+150)*lscaling + ldiv.upperBound();
	    setAxisScale(QwtPlot::yLeft,ldiv.lowerBound(),lyMax);

	    double rscaling = (rdiv.upperBound()-rdiv.lowerBound())/((double)graphRects.first().height()-1000);
	    double ryMax = static_cast<double>(rheight+150)*rscaling + rdiv.upperBound();
	    setAxisScale(QwtPlot::yRight,rdiv.lowerBound(),ryMax);
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

                //if bad zones are visible, they need to be recalculated after replotting
                QwtPlot::replot();

                bool replotAgain = false;
                if(!d_badTuneZones.isEmpty() && !d_hideBadZones)
                {
                    for(int i=0; i<d_badTuneZones.size(); i++)
                    {
                        if(d_badTuneZones.at(i).recalcWidthOnResize)
                        {
                            setZoneWidth(d_badTuneZones.at(i).zone,d_badTuneZones.at(i).md);
                            replotAgain = true;
                        }
                    }
                }
                if(replotAgain)
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
                for(int j=0;j<d_plotCurves.size();j++)
                {
                    if(j==curveIndex)
                    {
                        d_plotCurves[j]->setVisible(true);
                        static_cast<QwtLegendLabel*>(leg->legendWidget(itemToInfo(d_plotCurves[j])))->show();
                        if(autoYRanges)
                            setAxisScale(QwtPlot::yLeft,d_plotCurveMetaData.at(j).yMin,d_plotCurveMetaData.at(j).yMax);
                    }
                    else
                    {
                        d_plotCurves[j]->setVisible(false);
                        static_cast<QwtLegendLabel*>(leg->legendWidget(itemToInfo(d_plotCurves[j])))->hide();
                    }
                }
                //if bad zones are visible, they need to be recalculated after replotting
                QwtPlot::replot();

                bool replotAgain = false;
                if(!d_badTuneZones.isEmpty() && !d_hideBadZones)
                {
                    for(int i=0; i<d_badTuneZones.size(); i++)
                    {
                        if(d_badTuneZones.at(i).recalcWidthOnResize)
                        {
                            setZoneWidth(d_badTuneZones.at(i).zone,d_badTuneZones.at(i).md);
                            replotAgain = true;
                        }
                    }
                }
                if(replotAgain)
                    QwtPlot::replot();

                rend.render(this,&p,graphRects.at(rect));

                //break out of the plotting loop if we're done
                if(xMax >= end && curveIndex==d_plotCurves.size())
                    break;

                //increment curveIndex. if we've done the last one, reset it to 0 and increment the curve count
                curveIndex = (curveIndex+1) % d_plotCurves.size();
                if(curveIndex==0)
                    curveCount++;

            }
        }
    }

    p.end();

    if(oneCurvePerPlot)
    {
        for(int i=0; i<d_plotCurves.size(); i++)
        {
            d_plotCurves[i]->setVisible(true);
            static_cast<QwtLegendLabel*>(leg->legendWidget(itemToInfo(d_plotCurves[i])))->show();
        }
    }
}


