#include "drplot.h"

#include <QMouseEvent>
#include <QColorDialog>

#include <qwt6/qwt_legend.h>

DrPlot::DrPlot(int num, QWidget *parent) :
    AbstractBatchPlot(QString("drPlot"),parent)
{
    QFont labelFont(QString("sans serif"),8);
    QwtText plotTitle(QString("DR Scan %1").arg(num));
    plotTitle.setFont(labelFont);

    setTitle(plotTitle);

    QwtText xText(QString("Frequency (MHz)"));
    setAxisTitle(QwtPlot::xBottom,xText);
}



void DrPlot::receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
    if(d_metaDataList.isEmpty())
    {
        QwtLegend *l = new QwtLegend;
        l->setDefaultItemMode(QwtLegendData::Checkable);
        l->installEventFilter(this);
        connect(l,&QwtLegend::checked,this,&DrPlot::toggleCurve);
        insertLegend(l);

        QSettings s;
        s.beginGroup(QString("DrRanges"));
        for(int i=0; i<d.size(); i++)
        {
            QColor color = s.value(QString("DrRange%1").arg(QString::number(i)),
                               QPalette().color(QPalette::ToolTipBase)).value<QColor>();

            QwtPlotCurve *c = new QwtPlotCurve(QString("DrRange%1").arg(QString::number(i)));
            c->setRenderHint(QwtPlotItem::RenderAntialiased);
            c->setPen(QPen(color));
            d_plotCurves.append(c);
            c->setLegendAttribute(QwtPlotCurve::LegendShowLine,true);
            c->attach(this);
            PlotCurveMetaData drmd;
            drmd.curve = c;
            drmd.visible = true;
            drmd.yMin = d.at(i).at(0).y();
            drmd.yMax = d.at(i).at(0).y();
            d_plotCurveMetaData.append(drmd);

            setAxisAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);

            d_plotCurveData.append(QVector<QPointF>());

        }
        s.endGroup();
    }

    //only add the data if this is not a calibration!
    if(!md.isCal)
    {
        for(int i=0; i<d.size() && i<d_plotCurves.size(); i++)
        {
            for(int j=d_plotCurveData.at(i).size(); j<d.at(i).size(); j++)
            {
                d_plotCurveMetaData[i].yMin = qMin(d_plotCurveMetaData.at(i).yMin,d.at(i).at(j).y());
                d_plotCurveMetaData[i].yMax = qMax(d_plotCurveMetaData.at(i).yMax,d.at(i).at(j).y());
            }
            d_plotCurves[i]->setSamples(d.at(i));
            expandAutoScaleRange(QwtPlot::yLeft,d_plotCurveMetaData.at(i).yMin,d_plotCurveMetaData.at(i).yMax);
        }

        expandAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);
        d_plotCurveData = d;
    }

    d_metaDataList.append(md);
    if(d_showZonePending && !md.isCal)
    {
        d_showZonePending = false;
        if(md.scanNum == d_zoneScanNum)
            setSelectedZone(d_zoneScanNum);
    }

    replot();
}

void DrPlot::print()
{
    //set up options dialog
    QDialog d;
    d.setWindowTitle(QString("DR Scan Printing Options"));

    //get visibility status of all curves
    QList<bool> curveWasVisible;
    for(int i=0; i<d_plotCurveMetaData.size(); i++)
    {
        curveWasVisible.append(d_plotCurveMetaData.at(i).visible);
        if(!d_plotCurveMetaData.at(i).visible)
            toggleCurve(itemToInfo(d_plotCurveMetaData.at(i).curve),false,0);
    }

    QPair<double,double> autoScaleXRange = getAxisAutoScaleRange(QwtPlot::xBottom);
    QPair<double,double> autoScaleYRange = getAxisAutoScaleRange(QwtPlot::yLeft);

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
    if(d_plotCurves.size()==1)
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
    for(int i=0; i<d_plotCurves.size(); i++)
    {
        pens.append(d_plotCurves.at(i)->pen());
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

            d_plotCurves[i]->setPen(QPen(c,1.0));
        }
        else //otherwise, it's black
        {
            d_plotCurves[i]->setPen(Qt::black,1.0);
        }
        d_plotCurves[i]->setRenderHint(QwtPlotItem::RenderAntialiased,true);
        d_plotCurves[i]->setSamples(d_plotCurveData.at(i));
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
        if(d_metaDataList.size()>4)
        {
            //calculate step size. keep in mind that cal scans (taken between points) aren't plotted!
            double stepSize = 0.0;
            if(d_metaDataList.at(0).isCal)
                stepSize = qAbs((d_metaDataList.at(3).maxXVal-d_metaDataList.at(1).maxXVal)/2.0);
            else
                stepSize = qAbs(d_metaDataList.at(1).maxXVal-d_metaDataList.at(0).maxXVal);

            //calculate number of steps needed before labels won't overlap
            labelSpacingSteps = (int)ceil(labelWidth/stepSize) + 1;
        }
        for(int i=0; i<d_metaDataList.size(); i+=labelSpacingSteps)
        {
            if(d_metaDataList.at(i).isCal)
                i++;
            QwtText t(QString::number(d_metaDataList.at(i).scanNum));
            t.setFont(f);
            t.setColor(Qt::black);
            t.setBackgroundBrush(QBrush(Qt::white));
            double pos = (d_metaDataList.at(i).maxXVal+d_metaDataList.at(i).minXVal)/2.0;
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
    bool zoneWasVisible = p_selectedZone->isVisible();
    if(zoneWasVisible)
        p_selectedZone->setVisible(false);

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
#ifdef QTFTM_FTM1
    t = QwtText(QString("%1 %2").arg(QString("FTM1")).arg(title().text()));
#endif

#ifdef QTFTM_FTM2
    t = QwtText(QString("%1 %2").arg(QString("FTM2")).arg(title().text()));
#endif

    setTitle(QString(""));

    if(!ownGraphsBox->isChecked() || d_metaDataList.size()==1)
        doPrint(xMin,xMax,xWidth,perPage,t.text(),&p);
    else
        doPrint(xMin,xMax,xWidth,perPage,t.text(),&p,true);

    setTitle(oldTitle);

    //now, undo everything
    QwtLegend *l = new QwtLegend;
    l->setDefaultItemMode(QwtLegendData::Checkable);
    l->installEventFilter(this);
    connect(l,&QwtLegend::checked,this,&DrPlot::toggleCurve);
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
        p_selectedZone->setVisible(true);

    for(int i=0; i<d_plotCurves.size(); i++)
        d_plotCurves[i]->setPen(pens.at(i));

    for(int i=0; i<curveWasVisible.size(); i++)
    {
        if(!curveWasVisible.at(i))
            toggleCurve(itemToInfo(d_plotCurveMetaData.at(i).curve),true,0);
    }

    replot();

    QApplication::restoreOverrideCursor();

}

bool DrPlot::eventFilter(QObject *obj, QEvent *ev)
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
                    s.beginGroup(QString("DrRanges"));
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
