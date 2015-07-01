#include "surveyplot.h"

#include <qwt6/qwt_symbol.h>

SurveyPlot::SurveyPlot(int num, QWidget *parent) :
    AbstractBatchPlot(QString("surveyPlot"),parent)
{
    QFont labelFont(QString("sans serif"),8);
    QwtText plotTitle(QString("Survey %1").arg(num));
    plotTitle.setFont(labelFont);

    setTitle(plotTitle);

    QwtText xText(QString("Frequency (MHz)"));
    setAxisTitle(QwtPlot::xBottom,xText);
}


void SurveyPlot::receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d)
{
    if(d_metaDataList.isEmpty())
    {
        //this is the first chunk of data... initialize data storage
        p_calCurve = new QwtPlotCurve(QString("Calibration"));
        p_calCurve->setAxes(QwtPlot::xBottom,QwtPlot::yRight);
        p_calCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        //color from settings...
        QColor highlight = QPalette().color(QPalette::Highlight);
        p_calCurve->setPen(QPen(highlight));
        p_calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(highlight),QPen(highlight),QSize(5,5)));

        //for a survey, there will only be one data curve
        QwtPlotCurve *curve = new QwtPlotCurve(QString("Survey"));
        curve->setPen(QPen(QPalette().color(QPalette::Text)));
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        d_plotCurves.append(curve);

        PlotCurveMetaData pcmd;
        pcmd.curve = curve;
        pcmd.visible = true;
        pcmd.yMin = 0.0;
        pcmd.yMax = d.first().at(0).y();
        d_plotCurveMetaData.append(pcmd);

        p_calCurve->attach(this);
        curve->attach(this);

        d_plotCurveData.append(QVector<QPointF>());
        setAxisAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);

    }

    if(md.isCal)
    {
        double max = 0.0;
        for(int i=d_calCurveData.size(); i<d.at(0).size(); i++)
            max = qMax(max,d.at(0).at(i).y());

        expandAutoScaleRange(QwtPlot::yRight,0.0,max);

        d_calCurveData = d.at(0);
        p_calCurve->setSamples(d.at(0));
    }
    else
    {
        double max = 0.0;
        for(int i=d_plotCurveData.at(0).size(); i<d.at(0).size(); i++)
            max = qMax(max,d.at(0).at(i).y());

        d_plotCurveMetaData[0].yMax = qMax(max,d_plotCurveMetaData.at(0).yMax);
        expandAutoScaleRange(QwtPlot::yLeft,0.0,max);
        expandAutoScaleRange(QwtPlot::xBottom,md.minXVal,md.maxXVal);

        d_plotCurves[0]->setSamples(d.at(0));
        d_plotCurveData = d;
    }

    d_metaDataList.append(md);
    if(d_showZonePending)
    {
        d_showZonePending = false;
        if(md.scanNum == d_zoneScanNum)
            setSelectedZone(d_zoneScanNum);
    }

    replot();
}

void SurveyPlot::print()
{
    //set up options dialog
    QDialog d;
    d.setWindowTitle(QString("Survey Printing Options"));

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
    connect(box->button(QDialogButtonBox::Ok),&QPushButton::clicked,&d,&QDialog::accept);
    connect(box->button(QDialogButtonBox::Cancel),&QPushButton::clicked,&d,&QDialog::reject);
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
    QPen surveyPen(d_plotCurves.at(0)->pen());
    QPen calPen;
    if(!d_calCurveData.isEmpty())
        calPen = QPen(p_calCurve->pen());

    //survey curve settings
    //survey pen is always black
    d_plotCurves[0]->setPen(QPen(Qt::black,1.0));
    d_plotCurves[0]->setRenderHint(QwtPlotItem::RenderAntialiased);

    //cal curve settings
    if(!d_calCurveData.isEmpty() && printCalBox->isChecked())
    {
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

    //scan number markers
    if(scanNumbersBox->isChecked())
    {
        QFont f(QString("sans-serif"),7);
        QFontMetrics fm(f);

        double scaling = xWidth/(double)width();
        double labelWidth = (double)fm.boundingRect(QString("99999999")).width();
        double scanWidth = 1.0;
        int blah=0;
        while(blah < d_metaDataList.size())
        {
            if(d_metaDataList.at(blah).isCal)
            {
                blah++;
                continue;
            }
            scanWidth = d_metaDataList.at(blah).maxXVal-d_metaDataList.at(blah).minXVal;
            break;
        }
        labelWidth*= scaling;
        int labelSpacingSteps = ceil(1.5*labelWidth/scanWidth);
        for(int i=0; i<d_metaDataList.size(); i+=labelSpacingSteps)
        {
            if(d_metaDataList.at(i).isCal)
            {
                i++;
                if(i>=d_metaDataList.size())
                    break;
            }
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

    QPair<double,double> yRScale;
    yScale.first = axisScaleDiv(QwtPlot::yRight).lowerBound();
    yScale.second = axisScaleDiv(QwtPlot::yRight).upperBound();

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
#ifdef QTFTM_FTM1
    t = QwtText(QString("%1 %2").arg(QString("FTM1")).arg(title().text()));
#endif

#ifdef QTFTM_FTM2
    t = QwtText(QString("%1 %2").arg(QString("FTM2")).arg(title().text()));
#endif

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
    setAxisScale(QwtPlot::yRight,yRScale.first,yRScale.second);

    //remove markers and grid (this will delete them all if they're there)
    detachItems(QwtPlotItem::Rtti_PlotGrid);
    detachItems(QwtPlotItem::Rtti_PlotMarker);

    //restore curves
    if(zoneWasVisible)
        p_selectedZone->setVisible(true);

    d_plotCurves[0]->setPen(surveyPen);

    if(!d_calCurveData.isEmpty())
    {
        p_calCurve->setPen(calPen);
        p_calCurve->setVisible(true);
        p_calCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,QBrush(calPen.color()),QPen(calPen.color(),1.0),QSize(5,5)));
    }

    replot();

    QApplication::restoreOverrideCursor();

}
