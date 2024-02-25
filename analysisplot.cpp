#include "analysisplot.h"
#include "qpen.h"
#include <qwt6/qwt_plot_canvas.h>
#include <qwt6/qwt_plot_renderer.h>
#include <qwt6/qwt_scale_widget.h>
#include <qwt6/qwt_scale_map.h>
#include <qwt6/qwt_plot_grid.h>
#include <QColorDialog>
#include <QSettings>

AnalysisPlot::AnalysisPlot(QWidget *parent) :
	FtPlot(parent), activeDp(nullptr)
{
	//allow capturing of events on the plot canvas (mouse events, particularly)
	canvas()->installEventFilter(this);
}

AnalysisPlot::~AnalysisPlot()
{
}

void AnalysisPlot::startPeakMark(DopplerPair *d)
{
	//keep track of the doppler pair, put it on the plot, and start tracking the mouse
	activeDp = d;
	activeDp->attach(this);
	setDisplayType(ShowFt,true);
	canvas()->setMouseTracking(true);
	setMouseTracking(true);
}

void AnalysisPlot::showLoadedDopplerPair(DopplerPair *d)
{
	d->attach(this);
	setDisplayType(ShowFt,true);
}

void AnalysisPlot::clearDopplerPairs()
{
	//if there's an active doppler pair, it has been deleted elsewhere, so stop tracking it!
	if(activeDp)
	{
		canvas()->setMouseTracking(false);
		setMouseTracking(false);
		activeDp = 0;
	}

	replot();
}

void AnalysisPlot::selectFitColor()
{
	QSettings s;
	QColor oldColor = s.value(QString("fitCurveColor"),QColor(Qt::green)).value<QColor>();

	QColor newColor = QColorDialog::getColor(oldColor,this);
	if(newColor.isValid())
	{
		s.setValue(QString("fitCurveColor"),newColor);
		s.sync();
		setFitCurveColor();
	}
}

QMenu *AnalysisPlot::buildContextMenu()
{
	//get the menu from the base class
	QMenu *menu = FtPlot::buildContextMenu();

	menu->addSeparator();

	//add menu entry for peak marking
	QAction *markDopplerAction = menu->addAction(QString("Mark peak..."));
	connect(markDopplerAction,&QAction::triggered,this,&AnalysisPlot::peakMark);

	QAction *showScanDataAction = menu->addAction(QString("Show scan details"));
	connect(showScanDataAction,&QAction::triggered,this,&AnalysisPlot::showScanDetails);

	QAction *exportFtAction = menu->addAction(QString("Export FT..."));
	connect(exportFtAction,&QAction::triggered,this,&AnalysisPlot::exportFt);

	QAction *exportFidAction = menu->addAction(QString("Export FID..."));
	connect(exportFidAction,&QAction::triggered,this,&AnalysisPlot::exportFid);

	QMenu *autoFitMenu = new QMenu(QString("AutoFit"),menu);

	QAction *fitAction = autoFitMenu->addAction(QString("Do AutoFit..."));
	connect(fitAction,&QAction::triggered,this,&AnalysisPlot::autoFitRequested);

	QAction *paramAction = autoFitMenu->addAction(QString("Show fit params..."));
	connect(paramAction,&QAction::triggered,this,&AnalysisPlot::fitParametersRequested);

	QAction *logAction = autoFitMenu->addAction(QString("Show fit log..."));
	connect(logAction,&QAction::triggered,this,&AnalysisPlot::fitLogRequested);

	QAction *colorAction = autoFitMenu->addAction(QString("Change fit color..."));
	connect(colorAction,&QAction::triggered,this,&AnalysisPlot::selectFitColor);

    QAction *deleteAction = autoFitMenu->addAction(QString("Delete AutoFit"));
    connect(deleteAction,&QAction::triggered,this,&AnalysisPlot::autoFitDeleteRequested);

	menu->addMenu(autoFitMenu);

	//this would mean there's not a valid FT, so disable this action
	if(tracesHidden)
	{
		markDopplerAction->setEnabled(false);
		showScanDataAction->setEnabled(false);
		exportFtAction->setEnabled(false);
		exportFidAction->setEnabled(false);
		autoFitMenu->setEnabled(false);
	}

	return menu;

}

bool AnalysisPlot::eventFilter(QObject *obj, QEvent *ev)
{
	//event filtering is only needed for processing doppler pair actions
	if(!activeDp)
		return FtPlot::eventFilter(obj, ev);

	//doppler pair events will only be generated on the plot canvas
	if(obj == this->canvas())
	{
		//find relevant mouse events
		if(ev->type() == QEvent::MouseButtonPress)
		{
			//for a left click, finalize the peak marking session
			QMouseEvent *me = static_cast<QMouseEvent*>(ev);
			if(me->button() == Qt::LeftButton)
			{
				//null out activeDp pointer, accept event, and turn off mouse tracking
				ev->accept();
				canvas()->setMouseTracking(false);
				setMouseTracking(false);
                emit dopplerFinished(activeDp);
                activeDp = 0;
				return true;
			}
			else
				ev->ignore();
		}
		//wheel events adjust doppler pair spacing
		if(ev->type() == QEvent::Wheel)
		{
			//try to make the resolution of the spacing adjustment 1 pixel
			QWheelEvent *we = static_cast<QWheelEvent*>(ev);
			double conversionFactor = (xMax()-xMin())/(double)canvas()->width();

			//see QWheelEvent::delta() for explanation
			int d = we->delta()/120;

			//if control is pressed, make the splitting adjustment more coarse
			if(we->modifiers() & Qt::ControlModifier)
				d*=5;

			//calculate new splitting
			double newsplit = activeDp->splitting() + (double)d*conversionFactor;

			//make sure splitting is non-negative and it won't move the markers off the FT
			if(newsplit < 0.0)
				newsplit = 0.0;
			if(activeDp->center() - newsplit < xMin())
				newsplit = activeDp->center() - xMin();
			if(activeDp->center() + newsplit > xMax())
				newsplit = xMax() - activeDp->center();

			//update splitting
			if(newsplit != activeDp->splitting())
			{
				activeDp->split(newsplit);
				emit dopplerChanged(activeDp);
				replot();
                return true;
			}
			else
				ev->ignore();
		}

		//if the mouse moves, translate the doppler pair
		if(ev->type() == QEvent::MouseMove)
		{
			//convert the mouse x coordinate into x-axis value
			QMouseEvent *me = static_cast<QMouseEvent*>(ev);
            double xpos = canvasMap(QwtPlot::xBottom).invTransform(me->localPos().x());

			//only move if it won't move either component off the FT
			if(xpos - activeDp->splitting() >= xMin() && xpos + activeDp->splitting() <= xMax())
			{
				activeDp->moveCenter(xpos);
				emit dopplerChanged(activeDp);
				replot();
				ev->accept();
				return true;
			}
			else
				ev->ignore();
		}
	}

	return FtPlot::eventFilter(obj,ev);
}

void AnalysisPlot::contextMenuEvent(QContextMenuEvent *ev)
{
	//if there is no active doppler pair, make the menu
	if(!activeDp)
		return FtPlot::contextMenuEvent(ev);

	//otherwise, cancel the peak marking
	int num = activeDp->number();
	activeDp->detach();
	activeDp = 0;
	emit peakMarkCancelled(num);
	ev->accept();
	canvas()->setMouseTracking(false);
	setMouseTracking(false);
	replot();
}

void AnalysisPlot::renderTo(QPainter *p, QRect r)
{
	//make the plot renderer; don't print any background
	QwtPlotRenderer rend;
	rend.setDiscardFlags(QwtPlotRenderer::DiscardBackground | QwtPlotRenderer::DiscardCanvasBackground);

	//save curve pens and palettes
	QPen ftPen = ftCurve.pen();
	QPen fidPen = ftCurve.pen();
	QPalette axisPal = axisWidget(QwtPlot::xBottom)->palette();

	//attach a grid to the axes for printing
	QwtPlotGrid *pGrid = new QwtPlotGrid;
	pGrid->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
	pGrid->setMajorPen(QPen(QBrush(Qt::gray),1.0,Qt::DashLine));
	pGrid->setMinorPen(QPen(QBrush(Qt::lightGray),1.0,Qt::DotLine));
	pGrid->enableXMin(true);
	pGrid->enableYMin(true);
	pGrid->setRenderHint(QwtPlotItem::RenderAntialiased);
	pGrid->attach(this);

	//if there are any zones (integration ranges in DR) visible, hide them
	for(int i=0; i<itemList().size();i++)
	{
		if(itemList().at(i)->rtti() == QwtPlotItem::Rtti_PlotZone)
			itemList().at(i)->setVisible(false);
	}

	//make the curves black
	ftCurve.setPen(QPen(Qt::black,1.0));
	fidCurve.setPen(QPen(Qt::black,1.0));

	//make a palette with black foreground colors; apply to axes
	QPalette pal;
	pal.setColor(QPalette::Text,QColor(Qt::black));
	pal.setColor(QPalette::Foreground,QColor(Qt::black));

	axisWidget(QwtPlot::xBottom)->setPalette(pal);
	axisWidget(QwtPlot::yLeft)->setPalette(pal);

	//turn on antialiasing for prettier printing
	ftCurve.setRenderHint(QwtPlotItem::RenderAntialiased,true);
	fidCurve.setRenderHint(QwtPlotItem::RenderAntialiased,true);

	//the FT rectangle takes up the lower 3/4 of the total area
	QRect ftRect = r;
	ftRect.setHeight(r.height()*3/4);
	ftRect.moveTop(r.height()-r.height()*3/4);

	//the FID rectangle takes the upper 1/4
	QRect fidRect = r;
	fidRect.setHeight(r.height()/4);

	//remember the current view
	DisplayType viewType = d_type;

	//make sure FID is plotted, and render it
	displayFid();
	rend.render(this,p,fidRect);

	//now do the same for the FT
	displayFt();
	rend.render(this,p,ftRect);

	//set the display type back to what it was, but don't replot
	setDisplayType(viewType,false);

	//turn off antialiasing for faster screen display
	ftCurve.setRenderHint(QwtPlotItem::RenderAntialiased,false);
	fidCurve.setRenderHint(QwtPlotItem::RenderAntialiased,false);

	//restore colors, etc...
	axisWidget(QwtPlot::xBottom)->setPalette(axisPal);
	axisWidget(QwtPlot::yLeft)->setPalette(axisPal);
	ftCurve.setPen(ftPen);
	fidCurve.setPen(fidPen);

	for(int i=0; i<itemList().size();i++)
	{
		if(itemList().at(i)->rtti() == QwtPlotItem::Rtti_PlotZone)
			itemList().at(i)->setVisible(true);
	}

	//remove and delete grid
	pGrid->detach();
	delete pGrid;
}
