#include "batchattnplot.h"

#include <QMouseEvent>
#include <QColorDialog>

#include <qwt6/qwt_legend.h>

BatchAttnPlot::BatchAttnPlot(int num, QWidget *parent) :
    AbstractBatchPlot(QString("batchAttnPlot"),parent)
{
    QFont labelFont(QString("sans serif"),8);
    QwtText plotTitle(QString("Attenuation Scan %1").arg(num));
    plotTitle.setFont(labelFont);

    setTitle(plotTitle);

    QwtText xText(QString("Frequency (MHz)"));
    setAxisTitle(QwtPlot::xBottom,xText);
}



void BatchAttnPlot::receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
    if(d_metaDataList.isEmpty())
    {
        QwtLegend *l = new QwtLegend;
        l->setDefaultItemMode(QwtLegendData::Checkable);
        l->installEventFilter(this);
        connect(l,&QwtLegend::checked,this,&BatchAttnPlot::toggleCurve);
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
            c->setRenderHint(QwtPlotItem::RenderAntialiased);
            c->setPen(QPen(color));
            d_plotCurves.append(c);
            c->setLegendAttribute(QwtPlotCurve::LegendShowLine,true);
            c->attach(this);

            PlotCurveMetaData pcmd;
            pcmd.curve = c;
            pcmd.visible = true;
            pcmd.yMin = 0.0;
            pcmd.yMax = d.first().at(0).y();
            d_plotCurveMetaData.append(pcmd);

            d_plotCurveData.append(QVector<QPointF>());
            setAxisAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);

        }
        s.endGroup();
    }

    //recalculate min and max values for curves
    //note that we only need to compare new data, which is either at the beginning or end of d, depending on if scan is going up or down
    for(int i=0; i<d.size() && i<d_plotCurves.size(); i++)
    {
        if(d_metaDataList.isEmpty() || md.maxXVal>d_metaDataList.last().maxXVal)
        {
            for(int j=d_plotCurveData.at(i).size(); j<d.at(i).size(); j++)
            {
                d_plotCurveMetaData[i].yMin = qMin(d_plotCurveMetaData.at(i).yMin,d.at(i).at(j).y());
                d_plotCurveMetaData[i].yMax = qMax(d_plotCurveMetaData.at(i).yMax,d.at(i).at(j).y());
            }
        }
        else
        {
            for(int j=0; j<d.at(i).size() - d_plotCurveData.at(i).size(); j++)
            {
                d_plotCurveMetaData[i].yMin = qMin(d_plotCurveMetaData.at(i).yMin,d.at(i).at(j).y());
                d_plotCurveMetaData[i].yMax = qMax(d_plotCurveMetaData.at(i).yMax,d.at(i).at(j).y());
            }
        }
        d_plotCurves[i]->setSamples(d.at(i));
    }
    //set curve data
    d_plotCurveData = d;

    if(d_metaDataList.isEmpty() || md.maxXVal > d_metaDataList.last().maxXVal)
        d_metaDataList.append(md);
    else
        d_metaDataList.prepend(md);

    replot();
}

void BatchAttnPlot::print()
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
    for(int i=0; i<d_plotCurves.size(); i++)
    {
        pens.append(d_plotCurves.at(i)->pen());
        d_plotCurves[i]->setPen(Qt::black,1.0);
        d_plotCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased,true);
        d_plotCurves[i]->setSamples(d_plotCurveData.at(i));
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
    double xMin = d_plotCurveData.at(0).first().x();
    double xMax = d_plotCurveData.at(0).last().x();
    doPrint(xMin,xMax,xMax-xMin,1,t.text(),&p,true,true);

    //now, undo everything
    setTitle(oldTitle);

    QwtLegend *l = new QwtLegend;
    l->setDefaultItemMode(QwtLegendData::Checkable);
    l->installEventFilter(this);
    connect(l,&QwtLegend::checked,this,&BatchAttnPlot::toggleCurve);
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

    for(int i=0; i<d_plotCurves.size(); i++)
    {
        d_plotCurves[i]->setPen(pens.at(i));
        d_plotCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased,false);
        d_plotCurves[i]->setVisible(true);
    }

    replot();

    QApplication::restoreOverrideCursor();
}

bool BatchAttnPlot::eventFilter(QObject *obj, QEvent *ev)
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
                    s.beginGroup(QString("AttnCurves"));
                    c->setPen(QPen(color));
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
    return AbstractBatchPlot::eventFilter(obj,ev);
}
