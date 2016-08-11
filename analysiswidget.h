#ifndef ANALYSISWIDGET_H
#define ANALYSISWIDGET_H

#include <QWidget>
#include "ui_analysiswidget.h"
#include "dopplerpair.h"
#include "linelistmodel.h"
#include "analysisplot.h"
#include "scan.h"
#include <QFrame>
#include <QThread>
#include "fitresult.h"

namespace Ui {
class AnalysisWidget;
}

class FtPlot;
class DrIntSetupPage;

class AnalysisWidget : public QWidget
{
	Q_OBJECT
	
public:
    friend class DrIntSetupPage;
	explicit AnalysisWidget(QWidget *parent = nullptr);
	~AnalysisWidget();

	FtPlot *plot();
    int numLines() const { return llm->rowCount(); }
    QList<QPair<double,double> > ranges() { return llm->getRanges(); }
    void configureForDr();

    void setPlotDelay(const double d);
    void setPlotHpf(const double h);
    void setPlotExp(const double e);
    void setPlotRdc(const bool b);
    void setPlotPad(const bool b);
    void setPlotUseWindow(const bool b);

signals:
    void canPrint(bool);
    void scanChanged(int, bool replot = true);
    void linesChanged();
    void metaDataChanged(int);
    void peakAddRequested(int,double,double,QString s = QString("Added manually"));

private:
    Ui::AnalysisWidget *ui;
    LineListModel *llm;
    Scan d_currentScan;

    QFrame *renderPrintSidebar();
    QThread *fitThread;
    QPair<double,double> d_currentBaseline;

public slots:
    void peakMarkRequested();
    void buildLineListContextMenu(const QPoint p);
    void deleteLine();
    void newScan(Scan s);
    void showLatestScan();
    void loadScan(int num);
    void showScan(Scan s);
    void print();
    void clearLines();
    void showScanDetails();

    void enableSelection(bool enabled);
    void limitRange(int first, int last);
    void saveScanMetaData();
    void loadScanMetaData();
    void checkForLoadScanMetaData(int scanNum);
    void exportFt();
    void exportFid();
    void autoFit();
    void autoFitComplete(const FitResult &res);
    void deleteAutoFit();
    void viewFitResults();
    void viewFitLog();
    void setDopplerAmplitude(DopplerPair *d);
    void requestPeakAdd();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
};

#endif // ANALYSISWIDGET_H
