#include "batchscanplot.h"

#include <qwt6/qwt_symbol.h>

BatchScanPlot::BatchScanPlot(int num, QWidget *parent) :
    AbstractBatchPlot(QString("batchScanPlot"),parent)
{
    QFont labelFont(QString("sans serif"),8);
    QwtText plotTitle(QString("Batch Scan %1").arg(num));
    plotTitle.setFont(labelFont);

    setTitle(plotTitle);

    QwtText xText(QString("Scan number"));
    setAxisTitle(QwtPlot::xBottom,xText);
}



void BatchScanPlot::receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d)
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

//        expandAutoScaleRange(QwtPlot::yLeft,0.0,d_plotCurveMetaData.at(0).yMax);
//        if(!d_hideBatchLabels)
//            expandAutoScaleRange(QwtPlot::yLeft,0.0,calculateAxisMaxWithLabel(QwtPlot::yLeft));

        d_plotCurves[0]->setSamples(d.at(0));
        d_plotCurveData = d;
    }
    else
    {
        double max = 0.0;
        for(int i=d_calCurveData.size(); i<d.at(0).size(); i++)
            max = qMax(max,d.at(0).at(i).y());

//        expandAutoScaleRange(QwtPlot::yRight,0.0,max);
//        if(!d_hideBatchLabels)
//            expandAutoScaleRange(QwtPlot::yRight,0.0,calculateAxisMaxWithLabel(QwtPlot::yRight));


        d_calCurveData = d.at(0);
        p_calCurve->setSamples(d.at(0));
    }

    expandAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);

    if(d_showZonePending)
    {
        d_showZonePending = false;
        if(md.scanNum == d_zoneScanNum)
            setSelectedZone(d_zoneScanNum);
    }

    replot();

}

void BatchScanPlot::print()
{
    //set up options dialog
    QDialog d;
    d.setWindowTitle(QString("Batch Printing Options"));

    QPair<double,double> autoScaleYRange = getAxisAutoScaleRange(QwtPlot::yLeft);
    double ym = d_plotCurveMetaData.first().yMax;

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
    scanMinBox->setMinimum(d_metaDataList.first().scanNum);
    scanMinBox->setMaximum(d_metaDataList.last().scanNum);
    scanMinBox->setValue(d_metaDataList.first().scanNum);
    scanMinBox->setSingleStep(1);
    gbl->addRow(QString("First scan"),scanMinBox);

    QSpinBox *scanMaxBox = new QSpinBox(graphBox);
    scanMaxBox->setToolTip(QString("Ending scan"));
    scanMaxBox->setMinimum(d_metaDataList.first().scanNum);
    scanMaxBox->setMaximum(d_metaDataList.last().scanNum);
    scanMaxBox->setValue(d_metaDataList.last().scanNum);
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
    yMinBox->setMaximum(ym*0.9);
    yMinBox->setValue(0.0);
    yMinBox->setSingleStep(ym*0.1);
    gbl->addRow(QString("Y min"),yMinBox);

    QDoubleSpinBox *yMaxBox = new QDoubleSpinBox(graphBox);
    yMaxBox->setToolTip(QString("Maximum value on y axis of each graph"));
    yMaxBox->setDecimals(3);
    yMaxBox->setMinimum(0.0);
    yMaxBox->setMaximum(10.0*ym);
    yMaxBox->setValue(ym);
    yMaxBox->setSingleStep(ym*0.1);
    gbl->addRow(QString("Y max"),yMaxBox);

    graphBox->setLayout(gbl);

    //batch-related options
    QGroupBox *optionsBox = new QGroupBox("Options",&d);
    QFormLayout *obl = new QFormLayout(optionsBox);

    QCheckBox *colorCheckBox  = new QCheckBox(optionsBox);
    colorCheckBox->setToolTip(QString("If checked, graphs will be printed in color. The program will attempt to\ndarken any light foreground colors to provide good contrast on paper"));
    colorCheckBox->setChecked(false);
    obl->addRow(QString("Color"),colorCheckBox);

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
        start = (double)d_metaDataList.first().scanNum-0.5;
        end = (double)d_metaDataList.last().scanNum+0.5;
    }
    if(start > end)
        qSwap(start,end);

    if(yMin == yMax)
    {
        yMin = autoScaleYRange.first;
	   yMax = ym;
    }
    if(yMin > yMax)
        qSwap(yMin,yMax);

    //store pen
    QPen curvePen =  d_plotCurves.at(0)->pen();

    //store text color
    QColor textColor = d_plotMarkers.at(0)->label().color();

    d_plotCurves[0]->setPen(QPen(Qt::black,1.0));
    d_plotCurves[0]->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen calPen;
    if(!d_calCurveData.isEmpty())
        calPen = p_calCurve->pen();

    //cal curve settings
    if(!d_calCurveData.isEmpty())
    {
        //if printing in color, make sure the color is darkish
        if(colorCheckBox->isChecked())
        {
            QColor c = calPen.color();
		  c.setRed(c.red()/2);
		  c.setBlue(c.blue()/2);
		  c.setGreen(c.green()/2);

            p_calCurve->setPen(QPen(c,1.0));
            p_calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(c),QPen(c,1.0),QSize(5,5)));
        }
        else //otherwise, it's black
        {
            p_calCurve->setPen(Qt::black,1.0);
            p_calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(Qt::black),QPen(Qt::black,1.0),QSize(5,5)));
        }
    }
    else if(!d_calCurveData.isEmpty())
        p_calCurve->setVisible(false);

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

    for(int i=0; i<d_plotMarkers.size(); i++)
    {
	   if(!d_hidePlotLabels)
        {
            d_plotMarkers[i]->setVisible(true);
            QwtText t = d_plotMarkers.at(i)->label();
            t.setBackgroundBrush(QBrush(Qt::white));
            t.setColor(Qt::black);
            d_plotMarkers[i]->setLabel(t);
            d_plotMarkers[i]->setRenderHint(QwtPlotItem::RenderAntialiased,true);
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

    QPair<double,double> yRScale;
    yRScale.first = axisScaleDiv(QwtPlot::yRight).lowerBound();
    yRScale.second = axisScaleDiv(QwtPlot::yRight).upperBound();

    auto yr = getAxisAutoScaleRange(QwtPlot::yRight);
    setAxisScale(QwtPlot::yRight,yr.first,yr.second);
    setAxisScale(QwtPlot::yLeft,yMin,yMax);

    //un-filter data
    d_plotCurves[0]->setSamples(d_plotCurveData.at(0));

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
    setAxisScale(QwtPlot::yRight,yRScale.first,yRScale.second);

    //remove grid (this will delete it if it's there)
    detachItems(QwtPlotItem::Rtti_PlotGrid);

    //restore curves
    if(zoneWasVisible)
        p_selectedZone->setVisible(true);

    d_plotCurves[0]->setPen(curvePen);

    for(int i=0; i<d_plotMarkers.size(); i++)
    {
        QwtText t = d_plotMarkers.at(i)->label();
        t.setBackgroundBrush(QBrush());
        t.setColor(textColor);
        d_plotMarkers[i]->setLabel(t);
    }

    if(!d_calCurveData.isEmpty())
    {
        p_calCurve->setPen(calPen);
        p_calCurve->setVisible(true);
        p_calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(calPen.color()),QPen(calPen.color(),1.0),QSize(5,5)));
    }

    replot();

    QApplication::restoreOverrideCursor();

}
