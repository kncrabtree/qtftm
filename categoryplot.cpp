#include "categoryplot.h"

#include <qwt6/qwt_symbol.h>
#include <qwt6/qwt_plot_renderer.h>

CategoryPlot::CategoryPlot(int num, QWidget *parent) :
	AbstractBatchPlot(QString("catPlot"),parent)
{
	QFont labelFont(QString("sans serif"),8);
	QwtText plotTitle(QString("Category Batch %1").arg(num));
	plotTitle.setFont(labelFont);

	setTitle(plotTitle);

	QwtText xText(QString("Scan number"));
	setAxisTitle(QwtPlot::xBottom,xText);
}

CategoryPlot::~CategoryPlot()
{

}



void CategoryPlot::receiveData(QtFTM::BatchPlotMetaData md, QList<QVector<QPointF> > d)
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
		double max = 0.0;
	    for(int i=d_plotCurveData.at(0).size(); i<d.at(0).size(); i++)
	    {
		    max = qMax(max,d.first().at(i).y());
		   d_plotCurveMetaData[0].yMin = qMin(d_plotCurveMetaData.at(0).yMin,d.at(0).at(i).y());
	    }
	    d_plotCurveMetaData[0].yMax = qMax(d_plotCurveMetaData.at(0).yMax,max);
	    d_allMaxes.insert(md.scanNum,max);

	    if(md.isRef)
		    d_refMaxes.insert(md.scanNum,max);

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

	if(md.isRef && !md.badTune)
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

void CategoryPlot::print()
{
	if(d_metaDataList.isEmpty())
		return;

	//set up options dialog
	QDialog d;
	d.setWindowTitle(QString("Batch Printing Options"));

//	QPair<double,double> autoScaleYRange = getAxisAutoScaleRange(QwtPlot::yLeft);

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
	int start = scanMinBox->value(), end = scanMaxBox->value();
	int perPage = graphsPerPageBox->value();

	if(start == end)
	{
	    start = d_metaDataList.first().scanNum;
	    end = d_metaDataList.last().scanNum;
	}
	if(start > end)
	    qSwap(start,end);

	//store pen
	QPen curvePen =  d_plotCurves.at(0)->pen();

	//store text color
	QColor textColor = d_plotMarkers.at(0)->label().color();

	d_plotCurves[0]->setPen(QPen(Qt::black,1.0));

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

	//do printing -- this requires a different approach, so use custom function here.
	categoryPrint(&p,start,end,perPage,t.text());

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

	//un-filter data
	d_plotCurves[0]->setSamples(d_plotCurveData.at(0));

	replot();

	QApplication::restoreOverrideCursor();
}

void CategoryPlot::categoryPrint(QPrinter *pr, int firstScan, int lastScan, int graphsPerPage, QString title)
{
	//prepare the page layout and set the graph colors
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
	for(int i=0; i<graphsPerPage; i++)
	{
	    QRect r;
	    r.setWidth(pr->pageRect().width());
	    r.setHeight((pr->pageRect().height()-titleHeight)/graphsPerPage);
	    r.moveTop(i*r.height() + titleHeight);
	    graphRects.append(r);
	}

	auto yr = getAxisAutoScaleRange(QwtPlot::yRight);
	int height = 0;
	QFontMetrics fm3(d_plotMarkers.at(0)->label().font(),pr);
	for(int i=0;i<d_plotMarkers.size();i++)
	{
		if(d_metaDataList.at(i).isCal)
		{
			QString text = d_plotMarkers.at(i)->label().text();
			int numLines = text.split(QString("\n"),QString::SkipEmptyParts).size();
			height = qMax(height,fm3.boundingRect(text).height()*numLines);
		}
	}

	double scaling = yr.second/((double)graphRects.first().height()-1000);
	yr.second += static_cast<double>(height+150)*scaling;
	setAxisScale(QwtPlot::yRight,0.0,yr.second);

	//figure out number of pages
	//the idea is to end each row with a ref scan or a cal scan so that all tests for 1 line are plotted together.
	//The plot's left vertical scale is set to be equal to the ref line's max value.
	//if no ref is in the group (i.e., was aborted), we will use the max from the associated scans
	//Scans not associated with this line should be zeroed out

	//find index of first scan.
	int fIndex = -1;
	for(int i=0; i<d_metaDataList.size(); i++)
	{
		if(d_metaDataList.at(i).scanNum == firstScan)
		{
			fIndex = i;
			break;
		}
	}

	int lIndex = -1;
	for(int i=d_metaDataList.size()-1; i>=0; i--)
	{
		if(d_metaDataList.at(i).scanNum == lastScan)
		{
			lIndex = i;
			break;
		}
	}

	if(fIndex < 0 || lIndex < 0)
		return;

	//we need to find the associated reference scan for scaling
	//cal scans don't matter, as they're plotted on the right axis
	//get list of all reference scans in range, plus one past the end (if it exists)
	QList<int> refScansCopy = d_refMaxes.keys();
	QList<int> refScans;
	for(int i=0; i<refScansCopy.size(); i++)
	{
		if(refScansCopy.at(i) < fIndex)
			continue;

		refScans.append(refScansCopy.at(i));

		if(refScansCopy.at(i) > lIndex)
			break;
	}

	//refIndex will refer to the reference associated with the current scan
	//if refIndex is -1, then the graphs will be autoscaled to the largest scan in the group
	int refIndex = -1;
	int thisRefScan = -1;
	if(!refScans.isEmpty())
	{
		refIndex = 0;
		thisRefScan = refScans.at(refIndex);
	}

	//find first test scan associated with this group
	int firstCatScanNum = d_metaDataList.at(fIndex).scanNum;
	if(d_metaDataList.at(fIndex).isCal || d_metaDataList.at(fIndex).isRef)
	{
		int off = 1;
		while(fIndex + off <= lIndex)
		{
			if(!d_metaDataList.at(fIndex+off).isCal && !d_metaDataList.at(fIndex+off).isRef)
			{
				firstCatScanNum = d_metaDataList.at(fIndex+off).scanNum;
				break;
			}
		}
	}

	for(int i=thisRefScan; i>fIndex; i--)
	{
		if(d_metaDataList.at(i-1).isRef || d_metaDataList.at(i-1).isCal)
		{
			firstCatScanNum = d_metaDataList.at(i).scanNum;
			break;
		}
	}

	int currentPage = 0, currentRect = 0, currentPlotScan = 0, currentIndex = fIndex;
	QList<CategoryPrintGraph> graphList;

	CategoryPrintGraph currentGraphData;
	currentGraphData.yMax = d_refMaxes.value(d_metaDataList.at(refIndex).scanNum);
	currentGraphData.pageNum = currentPage;
	currentGraphData.rectNum = currentRect;
	currentGraphData.xMin = d_metaDataList.at(fIndex).scanNum - 0.5;
	currentGraphData.xMax = currentGraphData.xMin + 8.0;
	currentGraphData.refScanNum = d_metaDataList.at(refIndex).scanNum;
	currentGraphData.firstCatScanNum = firstCatScanNum;

	bool nextIsNewGraph = false;
	bool newGraphAfterCal = true;
	while(currentIndex <= lIndex)
	{
		bool newGraph = nextIsNewGraph;
		if(currentPlotScan >= 8 || nextIsNewGraph)
		{
			nextIsNewGraph = false;
			newGraphAfterCal = true;
			currentPlotScan = 0;
			newGraph = true;
		}

		if(d_metaDataList.at(currentIndex).isRef)
		{
			refIndex++;
			if(refIndex < refScans.size())
				thisRefScan = refScans.at(refIndex);
			else
				thisRefScan = -1;
			for(int i=currentIndex; i<lIndex; i++)
			{
				if(!d_metaDataList.at(i).isRef && !d_metaDataList.at(i).isCal)
					firstCatScanNum = d_metaDataList.at(i).scanNum;
			}

			//if the next scan is a cal scan, plot it on the same graph if there's room
			if(currentIndex+1 < lIndex)
			{
				if(d_metaDataList.at(currentIndex+1).isCal)
				{
					if(currentPlotScan + 1 < 8)
					{
						nextIsNewGraph = false;
						newGraphAfterCal = true;
					}
					else
					{
						nextIsNewGraph = true;
						newGraphAfterCal = false;
					}
				}
				else
				{
					nextIsNewGraph = true;
					newGraphAfterCal = true;
				}
			}
		}

		if(newGraph)
		{
			graphList.append(currentGraphData);
			currentRect++;
			if(currentRect > graphsPerPage)
			{
				currentRect = 0;
				currentPage++;
			}
			currentPlotScan = 0;
			if(thisRefScan >= 0)
				currentGraphData.yMax = d_refMaxes.value(thisRefScan);
			else
			{
				double max = 0.0;
				for(int i=currentIndex; i<lIndex; i++)
				{
					if(d_allMaxes.contains(i))
						max = qMax(max,d_allMaxes.value(i));
				}
				currentGraphData.yMax = max;
			}
			currentGraphData.pageNum = currentPage;
			currentGraphData.rectNum = currentRect;
			currentGraphData.xMin = d_metaDataList.at(currentIndex).scanNum - 0.5;
			currentGraphData.xMax = currentGraphData.xMin + 8.0;
			currentGraphData.refScanNum = thisRefScan;
			currentGraphData.firstCatScanNum = firstCatScanNum;
		}

		if(currentIndex == lIndex)
			graphList.append(currentGraphData);

		if(newGraphAfterCal && d_metaDataList.at(currentIndex).isCal)
			nextIsNewGraph = true;

		currentIndex++;
		currentPlotScan++;

	}

	int numPages;
	if(graphList.size() % graphsPerPage)
		numPages = graphList.size()/graphsPerPage + 1;
	else
		numPages = graphList.size()/graphsPerPage;

	QPainter p;
	p.begin(pr);

	int graphIndex = 0;
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

	    //loop over graph rectangles
	    for(int rect=0; rect<graphRects.size(); rect++)
	    {
		   double xMin = graphList.at(graphIndex).xMin;
		   double xMax = graphList.at(graphIndex).xMax;
		   setAxisScale(QwtPlot::xBottom,xMin,xMax);
		   double yMax = graphList.at(graphIndex).yMax;

		   height = 0;
		   for(int i=0;i<d_plotMarkers.size();i++)
		   {
			   if(!d_metaDataList.at(i).isCal)
			   {
				   QString text = d_plotMarkers.at(i)->label().text();
				   int numLines = text.split(QString("\n"),QString::SkipEmptyParts).size();
				   height = qMax(height,fm3.boundingRect(text).height()*numLines);
			   }
		   }

		   //estimating that the x axis scale/label take up ~1000 pts in printer scale
		   double scaling = yMax/((double)graphRects.at(rect).height()-1000);
		   yMax += static_cast<double>(height+150)*scaling;
		   setAxisScale(QwtPlot::yLeft,0.0,yMax);

		   for(int i=0; i<d_plotMarkers.size(); i++)
		   {
			   if(d_plotMarkers.at(i)->xValue() >= static_cast<double>(graphList.at(graphIndex).firstCatScanNum)-0.5 &&
					   d_plotMarkers.at(i)->xValue() <= static_cast<double>(graphList.at(graphIndex).refScanNum)+0.5)
				   d_plotMarkers.at(i)->setVisible(true);
			   else
				   d_plotMarkers.at(i)->setVisible(false);
		   }

		   //ignore any DR data that are not associated with this section
		   QVector<QPointF> currentCatData;
		   for(int i=0; i<d_plotCurveData.first().size(); i++)
		   {
			   if(d_plotCurveData.first().at(i).x() >= static_cast<double>(graphList.at(graphIndex).firstCatScanNum)-0.5 &&
					   d_plotCurveData.first().at(i).x() <= static_cast<double>(graphList.at(graphIndex).refScanNum)+0.5)
				   currentCatData.append(d_plotCurveData.first().at(i));
		   }
		   d_plotCurves[0]->setSamples(currentCatData);

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

		   graphIndex++;
		   if(graphIndex == graphList.size())
			  break;
	    }
	}

	p.end();
}
