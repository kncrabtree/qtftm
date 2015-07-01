#ifndef ANALYSISPLOT_H
#define ANALYSISPLOT_H

#include "ftplot.h"
#include "dopplerpair.h"
#include <QList>

class AnalysisPlot : public FtPlot
{
	Q_OBJECT
public:
	explicit AnalysisPlot(QWidget *parent = nullptr);
	~AnalysisPlot();

	double xMin() const { return currentFid.probeFreq(); }
	double xMax() const { if(currentFid.spacing() > 0.0) return currentFid.probeFreq() + 5e-7/currentFid.spacing(); else return 0.0; }
	virtual void renderTo(QPainter *p, QRect r);

	QVector<QPointF> getFt() const { return d_currentFtXY; }
	QVector<QPointF> getFid() const { return d_currentFidXY; }
	FtPlot::DisplayType display() const { return d_type; }
	
signals:
	void peakMark();
	void peakMarkCancelled(int);
	void dopplerChanged(DopplerPair *d);
	void showScanDetails();
    void dopplerFinished(DopplerPair *d = nullptr);
	void exportFt();
	void exportFid();
	void autoFitRequested();
	void fitParametersRequested();
	void fitLogRequested();
	
public slots:
	void startPeakMark(DopplerPair *d);
	void showLoadedDopplerPair(DopplerPair *d);
	void clearDopplerPairs();
	void selectFitColor();

protected:
	DopplerPair *activeDp;

	virtual QMenu* buildContextMenu();	
	virtual bool eventFilter(QObject *obj, QEvent *ev);
	virtual void contextMenuEvent(QContextMenuEvent *ev);	


	
};

#endif // ANALYSISPLOT_H
