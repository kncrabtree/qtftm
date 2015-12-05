#ifndef FTPLOT_H
#define FTPLOT_H

#include <qwt6/qwt_plot.h>
#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_series_data.h>
#include "fid.h"
#include "ftworker.h"
#include "dopplerpair.h"
#include <QThread>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDoubleSpinBox>
#include <QCheckBox>

class QwtPlotTextLabel;

class FtPlot : public QwtPlot
{
	Q_OBJECT
public:
	explicit FtPlot(QWidget *parent = nullptr);
    virtual ~FtPlot();

	enum DisplayType { ShowFid, ShowFt };
	enum DisplayZoom { Detail, All };

	FtWorker* worker(){ return ftWorker; }

	double getDelay() const;
	double getHpf() const;
	double getExp() const;

    void setDelay(const double d);
    void setHpf(const double hpf);
    void setExp(const double e);
    void setRemoveDc(const bool rdc);
    void setPadding(const bool pad);

    QDoubleSpinBox* getDelayBox() const { return p_delayBox; }
    QDoubleSpinBox* getHpfBox() const { return p_hpfBox; }
    QDoubleSpinBox* getExpBox() const { return p_expBox; }
    QCheckBox* getRemoveDcBox() const { return p_removeDcCheckBox; }
    QCheckBox* getPadFidBox() const { return p_padFidCheckBox; }

    QVector<QPointF> currentFtXY() const { return d_currentFtXY; }
	
signals:
	void newFtMax(double);

public slots:
	void newFid(const Fid fid);
	void newFt(const QVector<QPointF> ft, double max);
	void newFit(const QVector<QPointF> fitData);
	void newDisplayFid(const QVector<QPointF> fid);
	void updatePlot();
	void setDisplayType(DisplayType t, bool forceReplot = false);
	void displayFid(){ setDisplayType(ShowFid); }
	void displayFt(){ setDisplayType(ShowFt); }
	void zoomDetail() { d_zoom = Detail; setDisplayType(d_type,true); }
	void zoomAll() { d_zoom = All; setDisplayType(d_type,true); }
	void setAutoScale(bool as = true) { d_verticalAutoScale = as; if(as) replot(); }
	void hideTraces();
	void reclaimSpinBoxes();
	void setFitCurveColor();

	//these functions are used in batch DR mode to allow an FTPlot
	//to show the integration ranges
	void clearRanges();
	void attachIntegrationRanges(QList<QPair<double,double> > p);
	void changeColor(QString itemName, QColor c);
    void setCalVisible(bool on);

	virtual void replot();

protected:
	QThread *ftThread;
	FtWorker *ftWorker;

	Fid currentFid;
	QVector<QPointF> d_currentFtXY, d_currentFidXY;
	int d_fidDisplayPoints;

	DisplayType d_type;
	DisplayZoom d_zoom;

	QwtPlotCurve ftCurve;
	QwtPlotCurve fidCurve;
	QwtPlotCurve fitCurve;
	QwtText fidXLabel;
	QwtText ftXLabel;
    QwtPlotTextLabel *p_calLabel;

	virtual void contextMenuEvent(QContextMenuEvent *ev);
	virtual QMenu* buildContextMenu();
	bool tracesHidden;

    QDoubleSpinBox *p_delayBox;
    QDoubleSpinBox *p_hpfBox;
    QDoubleSpinBox *p_expBox;
    QCheckBox *p_removeDcCheckBox;
    QCheckBox *p_padFidCheckBox;

	bool d_verticalAutoScale;
	double d_verticalScaleMax;
	double d_verticalZoomScale;

	bool eventFilter(QObject *obj, QEvent *ev);
	virtual void zoom(const QWheelEvent *we);

	
};

#endif // FTPLOT_H
