#include "ftplot.h"
#include "ftworker.h"
#include "qpen.h"
#include <QAction>
#include <QActionGroup>
#include <QPalette>
#include <math.h>
#include <qwt6/qwt_plot_zoneitem.h>
#include <qwt6/qwt_plot_textlabel.h>
#include <QSettings>
#include <QGridLayout>
#include <QLabel>
#include <QWidgetAction>

#include <qwt6/qwt_scale_div.h>

FtPlot::FtPlot(QWidget *parent) :
	QwtPlot(parent), d_fidDisplayPoints(0), d_type(ShowFt), d_zoom(All), tracesHidden(true),
	d_verticalAutoScale(true), d_verticalScaleMax(1.0), d_verticalZoomScale(1.0)
{
	ftThread = new QThread(this);
	ftWorker = new FtWorker();
	connect(ftWorker,&FtWorker::ftDone,this,&FtPlot::newFt);
	connect(ftWorker,&FtWorker::fidDone,this,&FtPlot::newDisplayFid);
    connect(ftThread,&QThread::finished,ftWorker,&QObject::deleteLater);

    setAxisAutoScale(QwtPlot::xBottom,false);
    setAxisAutoScale(QwtPlot::yLeft,false);

    setAxisScale(QwtPlot::yLeft,0,0,d_verticalScaleMax);

    canvas()->installEventFilter(this);

	QFont labelFont(QString("sans serif"),8);
	ftXLabel = QwtText(QString("Frequency (MHz)"));
	ftXLabel.setFont(labelFont);

	fidXLabel = QwtText(QString("Time (<span>&mu;</span>s)"));
	fidXLabel.setFont(labelFont);

	setAxisFont(QwtPlot::xBottom,labelFont);
	setAxisFont(QwtPlot::yLeft,labelFont);

	setDisplayType(d_type,true);

	QPen p(QColor(QPalette().text().color()),1.0);
	ftCurve.setPen(p);
	fidCurve.setPen(p);
	setFitCurveColor();
    ftCurve.setRenderHint(QwtPlotItem::RenderAntialiased);
    fidCurve.setRenderHint(QwtPlotItem::RenderAntialiased);
    fitCurve.setRenderHint(QwtPlotItem::RenderAntialiased);

    p_calLabel = new QwtPlotTextLabel();
    p_calLabel->setZ(10.0);
    QwtText calText(QString("CAL"));
    calText.setRenderFlags(Qt::AlignRight | Qt::AlignTop);
    calText.setBackgroundBrush(QBrush(QPalette().color(QPalette::Window)));
    calText.setColor(QPalette().color(QPalette::Text));
    QFont f(QString("monospace"),36);
    f.setBold(true);
    calText.setFont(f);
    p_calLabel->setText(calText);
    p_calLabel->setVisible(false);
    p_calLabel->attach(this);

	auto doubleVc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    p_delayBox = new QDoubleSpinBox(this);
    p_delayBox->setToolTip(QString("Sets beginning portion of FID to zero"));
    p_delayBox->setMinimum(0.0);
    p_delayBox->setMaximum(100.0);
    p_delayBox->setDecimals(1);
    p_delayBox->setValue(ftWorker->delay());
    p_delayBox->setSingleStep(1.0);
    p_delayBox->setKeyboardTracking(false);
    p_delayBox->setSpecialValueText(QString("Off"));
    p_delayBox->setSuffix(QString::fromUtf8(" μs"));
    p_delayBox->setVisible(false);
    connect(p_delayBox,doubleVc,ftWorker,&FtWorker::setDelay);
    connect(p_delayBox,doubleVc,this,&FtPlot::updatePlot);

    p_hpfBox = new QDoubleSpinBox(this);
    p_hpfBox->setToolTip(QString("Cutoff frequency for high pass filter applied to FID"));
    p_hpfBox->setMinimum(0.0);
    p_hpfBox->setMaximum(1000.0);
    p_hpfBox->setDecimals(0);
    p_hpfBox->setValue(ftWorker->hpf());
    p_hpfBox->setSingleStep(50.0);
    p_hpfBox->setKeyboardTracking(false);
    p_hpfBox->setSpecialValueText(QString("Off"));
    p_hpfBox->setSuffix(QString::fromUtf8(" kHz"));
    p_hpfBox->setVisible(false);
    connect(p_hpfBox,doubleVc,ftWorker,&FtWorker::setHpf);
    connect(p_hpfBox,doubleVc,this,&FtPlot::updatePlot);

    p_expBox = new QDoubleSpinBox(this);
    p_expBox->setToolTip(QString("Time constant for exponential decay convolved with FID"));
    p_expBox->setMinimum(0.0);
    p_expBox->setMaximum(1000.0);
    p_expBox->setDecimals(1);
    p_expBox->setValue(ftWorker->exp());
    p_expBox->setSingleStep(10.0);
    p_expBox->setKeyboardTracking(false);
    p_expBox->setWrapping(true);
    p_expBox->setSpecialValueText(QString("Off"));
    p_expBox->setSuffix(QString::fromUtf8(" μs"));
    p_expBox->setVisible(false);
    connect(p_expBox,doubleVc,ftWorker,&FtWorker::setExp);
    connect(p_expBox,doubleVc,this,&FtPlot::updatePlot);

    p_removeDcCheckBox = new QCheckBox(QString("Remove DC Offset"),this);
    p_removeDcCheckBox->setToolTip(QString("The average of the entire FID will be subtracted to remove any DC offset"));
    p_removeDcCheckBox->setChecked(ftWorker->removeDC());
    p_removeDcCheckBox->setVisible(false);
    connect(p_removeDcCheckBox,&QAbstractButton::toggled,ftWorker,&FtWorker::setRemoveDC);
    connect(p_removeDcCheckBox,&QAbstractButton::toggled,this,&FtPlot::updatePlot);

    p_padFidCheckBox = new QCheckBox(QString("Zero-pad FID"));
    p_padFidCheckBox->setToolTip(QString("Add zeroes to the end of the FID to increase apparent resolution"));
    p_padFidCheckBox->setChecked(ftWorker->autoPad());
    p_padFidCheckBox->setVisible(false);
    connect(p_padFidCheckBox,&QAbstractButton::toggled,ftWorker,&FtWorker::setAutoPad);
    connect(p_padFidCheckBox,&QAbstractButton::toggled,this,&FtPlot::updatePlot);

    p_useWindowCheckBox = new QCheckBox(QString("Apply B-H Window"));
    p_useWindowCheckBox->setToolTip(QString("Apply Blackman-Harris window function to FID."));
    p_useWindowCheckBox->setChecked(ftWorker->isUseWindow());
    p_useWindowCheckBox->setVisible(false);
    connect(p_useWindowCheckBox,&QCheckBox::toggled,ftWorker,&FtWorker::setUseWindow);
    connect(p_useWindowCheckBox,&QCheckBox::toggled,this,&FtPlot::updatePlot);

	ftWorker->moveToThread(ftThread);
	ftThread->start();
}

FtPlot::~FtPlot()
{
    p_calLabel->detach();
    delete p_calLabel;

	ftThread->quit();
	ftThread->wait();
	delete ftThread;
}

void FtPlot::newFid(const Fid fid)
{
	if(tracesHidden)
	{
		if(d_type == ShowFid)
			fidCurve.attach(this);
		else
		{
			ftCurve.attach(this);
			fitCurve.attach(this);
		}

		tracesHidden = false;
	}

    if(d_type == ShowFt)
	{
        if(d_zoom == Detail)
        {
            if(fabs(currentFid.probeFreq()-fid.probeFreq())>0.0001)
                setAxisScale(QwtPlot::xBottom,fid.probeFreq()+0.1,fid.probeFreq()+0.7);
        }
        else
        {
            if(fabs(currentFid.probeFreq()-fid.probeFreq())>0.0001 || fabs(currentFid.maxFreq()-fid.maxFreq())>0.0001)
                setAxisScale(QwtPlot::xBottom,fid.probeFreq(),fid.maxFreq());
        }
	}

	if(d_type == ShowFid)
	{
		if((currentFid.size() != fid.size()) || (currentFid.spacing() != fid.spacing()))
			setAxisScale(QwtPlot::xBottom,0.0,fid.size()*fid.spacing()*1e6);
	}

	currentFid = fid;
	QMetaObject::invokeMethod(ftWorker,"doFT",Q_ARG(Fid,fid));
}

void FtPlot::newFt(const QVector<QPointF> ft, double max)
{
	ftCurve.setSamples(ft);
	d_currentFtXY = ft;

	//calculate what the vertical scale max should be
	//the max of the FT shouldn't fall below 1/2 the scale.
	//if we need to vertically rescale, change such that the FT max is at 3/4 the the scale
	if(max < 0.5*d_verticalScaleMax || max > d_verticalScaleMax)
		d_verticalScaleMax = 4.0*max/3.0;

	if(!d_verticalAutoScale && d_verticalScaleMax < d_verticalZoomScale)
		d_verticalAutoScale = true;

	emit newFtMax(max);

	if(d_type == ShowFt)
		replot();
}

void FtPlot::newFit(const QVector<QPointF> fitData)
{
//	if(fitData.isEmpty())
//		fitCurve.setVisible(false);

	fitCurve.setSamples(fitData);
//	fitCurve.setVisible(true);
	if(d_type == ShowFt)
		replot();
}

void FtPlot::newDisplayFid(const QVector<QPointF> fid)
{
	fidCurve.setSamples(fid);
	if(d_fidDisplayPoints != fid.size() && d_type == ShowFid)
		setAxisScale(QwtPlot::xBottom,0.0,fid.size()*currentFid.spacing()*1e6);
	d_fidDisplayPoints = fid.size();
	d_currentFidXY = fid;

	if(d_type == ShowFid)
		replot();
}

void FtPlot::updatePlot()
{
	if(currentFid.size()>0)
		QMetaObject::invokeMethod(ftWorker,"doFT",Q_ARG(Fid,currentFid));
}

void FtPlot::setDisplayType(FtPlot::DisplayType t, bool forceReplot)
{
	if(t != d_type || forceReplot)
	{
		if(t == ShowFid)
		{
			ftCurve.detach();
			fitCurve.detach();
			fidCurve.attach(this);
			setAxisTitle(QwtPlot::xBottom,fidXLabel);
			setAxisScale(QwtPlot::xBottom,0.0,d_fidDisplayPoints*currentFid.spacing()*1e6);
		}
		else
		{
			fidCurve.detach();
			ftCurve.attach(this);
			fitCurve.attach(this);
			setAxisTitle(QwtPlot::xBottom,ftXLabel);
			if(d_zoom == Detail)
			{
				if(currentFid.probeFreq()>0.0)
                    setAxisScale(QwtPlot::xBottom,currentFid.probeFreq()+0.1,currentFid.probeFreq()+0.7);
				else
					setAxisAutoScale(QwtPlot::xBottom,true);
			}
			else
            {
                if(currentFid.probeFreq()>0.0 && currentFid.maxFreq() > currentFid.probeFreq())
                    setAxisScale(QwtPlot::xBottom,currentFid.probeFreq(),currentFid.maxFreq());
                else
                    setAxisAutoScale(QwtPlot::xBottom,true);
            }
		}

		d_type = t;

		if(currentFid.size() > 0)
			replot();
	}
}

void FtPlot::hideTraces()
{
	ftCurve.detach();
	fidCurve.detach();
	fitCurve.detach();
	tracesHidden = true;
	replot();
}

void FtPlot::reclaimSpinBoxes()
{
    p_delayBox->setParent(this);
    p_hpfBox->setParent(this);
    p_expBox->setParent(this);
    p_removeDcCheckBox->setParent(this);
    p_padFidCheckBox->setParent(this);
    p_useWindowCheckBox->setParent(this);

    p_delayBox->setVisible(false);
    p_hpfBox->setVisible(false);
    p_expBox->setVisible(false);
    p_removeDcCheckBox->setVisible(false);
    p_padFidCheckBox->setVisible(false);
    p_useWindowCheckBox->setVisible(false);

	sender()->deleteLater();
}

void FtPlot::setFitCurveColor()
{
	QSettings s;
	QColor c = s.value(QString("fitCurveColor"),QColor(Qt::green)).value<QColor>();
	fitCurve.setPen(c,1.0);
	if(d_type == ShowFt && !tracesHidden)
		replot();
}

void FtPlot::clearRanges()
{
	//todo: change this to zone item!
	detachItems(QwtPlotItem::Rtti_PlotZone,true);
}

void FtPlot::attachIntegrationRanges(QList<QPair<double, double> > p)
{
	QSettings s;
	s.beginGroup(QString("DrRanges"));
	for(int i=0; i<p.size(); i++)
	{
		QwtPlotZoneItem *zone = new QwtPlotZoneItem();
		zone->setOrientation(Qt::Vertical);
		QColor zoneColor = s.value(QString("DrRange%1").arg(QString::number(i)),
							  QPalette().color(QPalette::ToolTipBase)).value<QColor>();
		zoneColor.setAlpha(128);
		zone->setBrush(QBrush(zoneColor));
		zone->setAxes(QwtPlot::xBottom,QwtPlot::yLeft);
		zone->setInterval(p.at(i).first,p.at(i).second);
		zone->setTitle(QString("DrRange%1").arg(QString::number(i)));
		zone->attach(this);
	}
	s.endGroup();
}

void FtPlot::changeColor(QString itemName, QColor c)
{
	QwtPlotItemList l = itemList();
	for(int i=0; i<l.size(); i++)
	{
		if(itemName == l.at(i)->title().text())
		{
			c.setAlpha(128);
			QwtPlotZoneItem *z = static_cast<QwtPlotZoneItem*>(l[i]);
			if(z)
			{
				z->setBrush(QBrush(c));
				replot();
			}
		}
    }
}

void FtPlot::setCalVisible(bool on)
{
    p_calLabel->setVisible(on);
}

void FtPlot::replot()
{
	if(d_type == ShowFt)
	{
		setAxisAutoScale(QwtPlot::yLeft,false);
		double axisYMax = axisScaleDiv(QwtPlot::yLeft).upperBound();
		if(d_verticalAutoScale)
		{
            if(!qFuzzyCompare(axisYMax,d_verticalScaleMax))
				setAxisScale(QwtPlot::yLeft,0.0,d_verticalScaleMax);
		}
		else
		{
            if(!qFuzzyCompare(axisYMax,d_verticalZoomScale))
				setAxisScale(QwtPlot::yLeft,0.0,d_verticalZoomScale);
		}
	}
	else if(d_type == ShowFid)
		setAxisAutoScale(QwtPlot::yLeft,true);

	QwtPlot::replot();
}

void FtPlot::contextMenuEvent(QContextMenuEvent *ev)
{
	QMenu *menu = buildContextMenu();
	menu->popup(ev->globalPos());
	ev->accept();
}

QMenu *FtPlot::buildContextMenu()
{
	QMenu *out = new QMenu();
	connect(out,&QMenu::aboutToHide,this,&FtPlot::reclaimSpinBoxes);
	out->setAttribute(Qt::WA_DeleteOnClose);

	QMenu *displayMenu = new QMenu(QString("Display"),out);

	QActionGroup *displayGroup = new QActionGroup(displayMenu);
	QAction *showFidAction = displayGroup->addAction(QString("Show FID"));
	showFidAction->setCheckable(true);
	QAction *showFtAction = displayGroup->addAction(QString("Show FT"));
	showFtAction->setCheckable(true);
	displayGroup->setExclusive(true);
	if(d_type == ShowFt)
		showFtAction->setChecked(true);
	else
		showFidAction->setChecked(true);
	connect(showFidAction,&QAction::triggered,this,&FtPlot::displayFid);
	connect(showFtAction,&QAction::triggered,this,&FtPlot::displayFt);

	displayMenu->addAction(showFidAction);
	displayMenu->addAction(showFtAction);
	out->addMenu(displayMenu);

	QMenu *zoomMenu = new QMenu(QString("Zoom"),out);

	QActionGroup *zoomGroup = new QActionGroup(displayMenu);
	QAction *zoomAllAction = zoomGroup->addAction(QString("All"));
	zoomAllAction->setCheckable(true);
	QAction *zoomDetailAction = zoomGroup->addAction(QString("Detail"));
	zoomDetailAction->setCheckable(true);
	zoomGroup->setExclusive(true);
	if(d_zoom == All)
		zoomAllAction->setChecked(true);
	else
		zoomDetailAction->setChecked(true);
	connect(zoomAllAction,&QAction::triggered,this,&FtPlot::zoomAll);
	connect(zoomDetailAction,&QAction::triggered,this,&FtPlot::zoomDetail);

	zoomMenu->addAction(zoomAllAction);
	zoomMenu->addAction(zoomDetailAction);
	QAction *autoScaleAction = zoomMenu->addAction(QString("Autoscale vertical"));
	if(d_verticalAutoScale)
		autoScaleAction->setEnabled(false);
    connect(autoScaleAction,&QAction::triggered,[=](){ setAutoScale(true); });


	if(d_type == ShowFid)
		zoomMenu->setEnabled(false);
	out->addMenu(zoomMenu);

	QWidgetAction *processingAction = new QWidgetAction(out);
	QWidget *processingWidget = new QWidget(out);
	QGridLayout *processingLayout = new QGridLayout(processingWidget);

	processingLayout->addWidget(new QLabel(QString("Delay "),processingWidget),0,0);
    processingLayout->addWidget(p_delayBox,0,1);
    p_delayBox->setVisible(true);

	processingLayout->addWidget(new QLabel(QString("High pass "),processingWidget),1,0);
    processingLayout->addWidget(p_hpfBox,1,1);
    p_hpfBox->setVisible(true);

	processingLayout->addWidget(new QLabel(QString("Exp filter "),processingWidget),2,0);
    processingLayout->addWidget(p_expBox,2,1);
    p_expBox->setVisible(true);

    processingLayout->addWidget(p_useWindowCheckBox,3,0,1,2);
    p_useWindowCheckBox->setVisible(true);

    processingLayout->addWidget(p_removeDcCheckBox,4,0,1,2);
    p_removeDcCheckBox->setVisible(true);

    processingLayout->addWidget(p_padFidCheckBox,5,0,1,2);
    p_padFidCheckBox->setVisible(true);


	processingWidget->setLayout(processingLayout);
	processingAction->setDefaultWidget(processingWidget);
	out->addAction(processingAction);

	return out;
}

bool FtPlot::eventFilter(QObject *obj, QEvent *ev)
{
	if(obj == canvas())
	{
		if(ev->type() == QEvent::Wheel)
		{
			QWheelEvent *we = static_cast<QWheelEvent*>(ev);
			if(we)
			{
				zoom(we);
				we->accept();
				return true;
			}
		}
	}

	return QwtPlot::eventFilter(obj,ev);
}

void FtPlot::zoom(const QWheelEvent *we)
{
    int numSteps = we->angleDelta().y()/8/15;
	double scaleYMax = axisScaleDiv(QwtPlot::yLeft).upperBound();
	scaleYMax -= scaleYMax*0.1*numSteps;

	if(scaleYMax >= d_verticalScaleMax)
		setAutoScale(true);
	else
	{
		setAutoScale(false);
		d_verticalZoomScale = scaleYMax;
		replot();
	}
}

double FtPlot::getDelay() const
{
	double out = 0.0;
	QMetaObject::invokeMethod(ftWorker,"delay",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out));
	return out;
}

double FtPlot::getHpf() const
{
	double out = 0.0;
	QMetaObject::invokeMethod(ftWorker,"hpf",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out));
	return out;
}

double FtPlot::getExp() const
{
	double out = 0.0;
	QMetaObject::invokeMethod(ftWorker,"exp",Qt::BlockingQueuedConnection,Q_RETURN_ARG(double,out));
    return out;
}

bool FtPlot::isUseWindow() const
{
    bool out = false;
    QMetaObject::invokeMethod(ftWorker,"isUseWindow",Qt::BlockingQueuedConnection,Q_RETURN_ARG(bool,out));
    return out;
}

void FtPlot::setDelay(const double d)
{
    p_delayBox->setValue(d);
}

void FtPlot::setHpf(const double hpf)
{
    p_hpfBox->setValue(hpf);
}

void FtPlot::setExp(const double e)
{
    p_expBox->setValue(e);
}

void FtPlot::setRemoveDc(const bool rdc)
{
    p_removeDcCheckBox->setChecked(rdc);
}

void FtPlot::setPadding(const bool pad)
{
    p_padFidCheckBox->setChecked(pad);
}

void FtPlot::setUseWindow(const bool b)
{
    p_useWindowCheckBox->setChecked(b);
}
