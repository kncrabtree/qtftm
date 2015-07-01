#ifndef BATCHPLOT_H
#define BATCHPLOT_H

#include <qwt6/qwt_plot.h>
#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_plot_marker.h>
#include <qwt6/qwt_plot_picker.h>
#include <qwt6/qwt_plot_zoneitem.h>
#include <QList>
#include "batchmanager.h"
#include <QMouseEvent>
#include <QtPrintSupport/QPrinter>

class BatchPlot : public QwtPlot
{
	Q_OBJECT
public:
	explicit BatchPlot(QWidget *parent = nullptr);
    ~BatchPlot();
	
signals:
	void requestScan(int);
	void colorChanged(QString,QColor);
	
public slots:
    void prepareForNewBatch(BatchManager::BatchType t); //handled by constructor/destructor
    void receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d); // to base class
    void contextMenu(QPointF pos); // to base class; can be overridden
    void setBatchTitle(QString text); // Set in constructor --> int?
    void hideLabelsCallback(bool hide); // Batch only

    void loadCalScan(); //move to base class, give QPointF argument
    void loadScan(); //move to base class, give QPointF argument
    void showZone(int scanNum, bool doReplot=true); //move to base class
    void recalcZone(bool replot=true); //probably not needed
    void filterData(); //pure virtual
    void toggleCurve(QVariant item,bool hide,int index); //implement in base class
    void enableAutoScaling(); //handled by ZoomPanPlot
    void printCallback(); //virtual function in base class

    void disableReplotting() { d_replot = false; }
    void enableReplotting();
    void exportXY();

private:
	QList<QwtPlotCurve*> plotCurves;
	QList<QVector<QPointF> > plotCurveData;

	QList<QwtPlotMarker*> plotMarkers;
	QList<BatchManager::BatchPlotMetaData> metaData;
	QwtPlotCurve* calCurve;
	QVector<QPointF> calCurveData;

	QwtPlotZoneItem *zone;

	QPointF lastClickPos;
	bool showZonePending;
	int zoneScanNum;
	bool zoneIsSurveyCal;

	QwtText freqText;
	QwtText scanText;

	void surveyDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d);
	void drDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d);
	void batchDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d);
    void attnDataReceived(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d);

    bool d_replot;

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	void replot();

private:
	//begin section for data members and functions for zooming and panning
	bool panning;
	bool autoScaleXActive;
	bool autoScaleYActive;
	QList<QPair<double,double> > yMinMaxes;
	QPair<double,double> surveyCalMinMaxes;
	QPoint panClickPos;
	QPair<double,double> autoScaleYRange;
	QPair<double,double> autoScaleXRange;
    bool d_hideBatchLabels;

	void pan(QMouseEvent *me);
	void zoom(QWheelEvent *we);
	void autoScale();
	void resizeEvent(QResizeEvent *e);

	//printing functions
	void printSurvey();
	void printDr();
	void printBatch();
    void printAttn();

    void doPrint(double start, double end, double xRange, int plotsPerPage, QString title, QPrinter *pr, bool oneCurvePerPlot = false, bool autoYRanges = false);

};

#endif // BATCHPLOT_H
