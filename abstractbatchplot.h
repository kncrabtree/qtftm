#ifndef ABSTRACTBATCHPLOT_H
#define ABSTRACTBATCHPLOT_H

#include "zoompanplot.h"

#include <QMenu>

#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_plot_zoneitem.h>
#include <qwt6/qwt_plot_grid.h>
#include <qwt6/qwt_scale_widget.h>
//the following includes are for convenience for making print dialogs for implementations
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialog>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QLabel>
#include <QPainter>
#include <QPushButton>

#include "batchmanager.h"

class QPrinter;

class AbstractBatchPlot : public ZoomPanPlot
{
    Q_OBJECT
public:
    explicit AbstractBatchPlot(QString name, QWidget *parent = nullptr);
    ~AbstractBatchPlot();

    struct PlotCurveMetaData {
        QwtPlotCurve *curve;
        double yMin;
        double yMax;
        bool visible;
    };

    struct BadZone {
        QwtPlotZoneItem *zone;
        BatchManager::BatchPlotMetaData md;
        bool recalcWidthOnResize;
    };

signals:
    void requestScan(int);
    void colorChanged(QString,QColor);

public slots:
    virtual void receiveData(BatchManager::BatchPlotMetaData md, QList<QVector<QPointF> > d) =0;
    virtual void launchContextMenu(QPoint pos);

    virtual void loadScan(const double x); //default Implementation; can override
    virtual void loadCalScan(const double x); //default Implementation; can override
    virtual void setSelectedZone(int scanNum); // default implementation; can override for Batch
    virtual void formatSelectedZone(int metadataIndex); // default implementation; can override if needed
    virtual void setZoneWidth(QwtPlotZoneItem *zone,BatchManager::BatchPlotMetaData md);
    virtual void filterData();
    virtual void print() =0;
    virtual void exportXY();

    void disableReplotting() { d_doNotReplot = true; }
    void enableReplotting() { d_doNotReplot = false; replot(); }

    void toggleCurve(QVariant item, bool hide, int index);

protected:
    QList<QwtPlotCurve*> d_plotCurves;
    QList<QVector<QPointF>> d_plotCurveData;
    QwtPlotCurve *p_calCurve;
    QVector<QPointF> d_calCurveData;
    QwtPlotZoneItem *p_selectedZone;
    QList<BadZone> d_badTuneZones;
    QList<BatchManager::BatchPlotMetaData> d_metaDataList;
    QList<PlotCurveMetaData> d_plotCurveMetaData;

    int d_zoneScanNum;
    bool d_showZonePending;
    bool d_recalcZoneOnResize;
    bool d_doNotReplot;
    bool d_hideBadZones;

    void addBadZone(BatchManager::BatchPlotMetaData md);
    virtual QMenu *contextMenu();
    virtual bool eventFilter(QObject *obj, QEvent *ev);
    virtual void replot();

    void doPrint(double start, double end, double xRange, int plotsPerPage, QString title, QPrinter *pr, bool oneCurvePerPlot = false, bool autoYRanges = false);


};

#endif // ABSTRACTBATCHPLOT_H
