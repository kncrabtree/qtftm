#include "batchviewwidget.h"
#include "ui_batchviewwidget.h"
#include "batchsurvey.h"
#include "batchdr.h"
#include "batch.h"
#include "batchattenuation.h"
#include "drcorrelation.h"
#include "batchcategorize.h"
#include "surveyplot.h"
#include "drplot.h"
#include "batchscanplot.h"
#include "batchattnplot.h"
#include "drcorrplot.h"
#include "categoryplot.h"
#include "amdorwidget.h"
#include "amdorbatch.h"
#include <QThread>

BatchViewWidget::BatchViewWidget(QtFTM::BatchType type, int num, AbstractFitter *af, QWidget *parent) :
    QWidget(parent), ui(new Ui::BatchViewWidget), d_number(num), d_type(type)
{
    ui->setupUi(this);
    p_amdorWidget = nullptr;

    switch(d_type)
    {
    case QtFTM::Survey:
        batchPlot = new SurveyPlot(d_number,this);
        break;
    case QtFTM::DrScan:
        batchPlot = new DrPlot(d_number,this);
        break;
    case QtFTM::Batch:
        batchPlot = new BatchScanPlot(d_number,this);
        break;
    case QtFTM::Attenuation:
        batchPlot = new BatchAttnPlot(d_number,this);
        break;
    case QtFTM::DrCorrelation:
    case QtFTM::Amdor:
        batchPlot = new DrCorrPlot(d_number,d_type,this);
	    break;
    case QtFTM::Categorize:
	    batchPlot = new CategoryPlot(d_number,this);
	    break;
    default:
        break;
    }


    ui->batchSplitter->insertWidget(d_number,batchPlot);

    ui->afw->setFromFitter(af);

    connect(ui->printScanButton,&QAbstractButton::clicked,ui->analysisWidget,&AnalysisWidget::print);
    connect(ui->peakListWidget,&PeakListWidget::scanSelected,ui->analysisWidget,&AnalysisWidget::loadScan);
    connect(ui->analysisWidget,&AnalysisWidget::scanChanged,ui->peakListWidget,&PeakListWidget::selectScan);
    connect(ui->analysisWidget,&AnalysisWidget::peakAddRequested,ui->peakListWidget,&PeakListWidget::addUniqueLine);
    connect(ui->reprocessButton,&QAbstractButton::clicked,this,&BatchViewWidget::process);
    connect(this,&BatchViewWidget::checkForMetaDataChanged,ui->analysisWidget,&AnalysisWidget::checkForLoadScanMetaData);
    connect(ui->analysisWidget,&AnalysisWidget::metaDataChanged,this,&BatchViewWidget::metaDataChanged);

    ui->reprocessButton->setEnabled(false);
    ui->printBatchButton->setEnabled(false);
    ui->printScanButton->setEnabled(false);
    ui->analysisWidget->enableSelection(false);

    batchThread = new QThread();

    setAttribute(Qt::WA_DeleteOnClose,true);

}

BatchViewWidget::~BatchViewWidget()
{
    batchThread->quit();
    batchThread->wait();
    delete batchThread;

    delete ui;
}

void BatchViewWidget::process()
{
    if(batchThread->isRunning())
        return;

    ui->printBatchButton->setEnabled(false);
    ui->printScanButton->setEnabled(false);
    ui->reprocessButton->setEnabled(false);
    ui->analysisWidget->loadScan(0);
    ui->analysisWidget->limitRange(0,0);
    ui->analysisWidget->enableSelection(false);


    ui->statusLabel->setText(QString("Processing..."));
    setCursor(Qt::BusyCursor);

    BatchManager *bm;
    AmdorBatch *ab;

    switch(d_type)
    {
    case QtFTM::Survey:
        bm = new BatchSurvey(d_number,ui->afw->toFitter());
        break;
    case QtFTM::DrScan:
        bm = new BatchDR(d_number,ui->afw->toFitter());
        break;
    case QtFTM::Batch:
        bm = new Batch(d_number,ui->afw->toFitter());
        break;
    case QtFTM::Attenuation:
        bm = new BatchAttenuation(d_number);
        break;
    case QtFTM::DrCorrelation:
        bm = new DrCorrelation(d_number,ui->afw->toFitter());
	    break;
    case QtFTM::Categorize:
        bm = new BatchCategorize(d_number,ui->afw->toFitter());
	    break;
    case QtFTM::Amdor:
        if(p_amdorWidget != nullptr)
        {
            ui->tabWidget->removeTab(ui->tabWidget->count()-1);
            p_amdorWidget->deleteLater();
        }

        ab = new AmdorBatch(d_number,ui->afw->toFitter());
        bm = ab;
        p_amdorWidget = new AmdorWidget(AmdorData(ab->allFrequencies(),ab->matchThreshold()),d_number,this);
        ui->tabWidget->addTab(p_amdorWidget,QIcon(QString(":/icons/amdor.png")),QString("AMDOR"));
        connect(ab,&AmdorBatch::newRefScan,p_amdorWidget,&AmdorWidget::newRefScan);
        connect(ab,&AmdorBatch::newDrScan,p_amdorWidget,&AmdorWidget::newDrScan);
        p_amdorWidget->livePlotting(false);
        p_amdorWidget->enableEditing(false);
        break;
    case QtFTM::SingleScan:
    default:
        ui->statusLabel->setText(QString("Somehow, an invalid batch type was selected. Please close and try again."));
        unsetCursor();
        return;
    }



    QPair<int,int> range = bm->loadScanRange();
    if((range.first < 1 || range.second < 1) && d_type != QtFTM::Attenuation)
    {
        ui->statusLabel->setText(QString("%1 could not be read from disk.").arg(bm->title()));
        unsetCursor();
        return;
    }


    d_firstScan = range.first;
    d_lastScan = range.second;
    setWindowTitle(bm->title());


    connect(batchThread,&QThread::started,bm,&BatchManager::beginBatch);
    connect(bm,&BatchManager::batchComplete,this,&BatchViewWidget::processingComplete);
    connect(bm,&BatchManager::batchComplete,batchThread,&QThread::quit);
    connect(bm,&BatchManager::titleReady,ui->peakListWidget,&PeakListWidget::setTitle);
    connect(bm,&BatchManager::processingComplete,ui->peakListWidget,&PeakListWidget::addScan);
    connect(batchThread,&QThread::finished,bm,&QObject::deleteLater);

    ui->analysisWidget->plot()->clearRanges();
    ui->peakListWidget->clearAll();

    if(d_type == QtFTM::DrScan)
    {
        QList<QPair<double,double> > ranges = dynamic_cast<BatchDR*>(bm)->integrationRanges();
        ui->analysisWidget->plot()->attachIntegrationRanges(ranges);
    }

    bm->moveToThread(batchThread);

    QByteArray state = ui->batchSplitter->saveState();
    delete batchPlot;
    switch(d_type)
    {
    case QtFTM::Survey:
        batchPlot = new SurveyPlot(d_number,this);
        break;
    case QtFTM::DrScan:
        batchPlot = new DrPlot(d_number,this);
        break;
    case QtFTM::Batch:
        batchPlot = new BatchScanPlot(d_number,this);
        break;
    case QtFTM::Attenuation:
        batchPlot = new BatchAttnPlot(d_number,this);
        break;
    case QtFTM::DrCorrelation:
    case QtFTM::Amdor:
        batchPlot = new DrCorrPlot(d_number,d_type,this);
	    break;
    case QtFTM::Categorize:
	    batchPlot = new CategoryPlot(d_number,this);
	    break;
    default:
        break;
    }
    batchPlot->disableReplotting();

    connect(bm,&BatchManager::plotData,batchPlot,&AbstractBatchPlot::receiveData);
    connect(ui->printBatchButton,&QAbstractButton::clicked,batchPlot,&AbstractBatchPlot::print);
    connect(ui->analysisWidget,&AnalysisWidget::scanChanged,batchPlot,&AbstractBatchPlot::setSelectedZone);
    connect(batchPlot,&AbstractBatchPlot::requestScan,ui->analysisWidget,&AnalysisWidget::loadScan);
    connect(batchPlot,&AbstractBatchPlot::colorChanged,ui->analysisWidget->plot(),&FtPlot::changeColor);

    ui->batchSplitter->insertWidget(1,batchPlot);
    ui->batchSplitter->restoreState(state);


    batchThread->start();

}

void BatchViewWidget::processingComplete(bool failure)
{
    ui->reprocessButton->setEnabled(!failure);
    ui->printBatchButton->setEnabled(!failure);
    ui->printScanButton->setEnabled(!failure);
    ui->analysisWidget->enableSelection(!failure);

    if(failure)
        ui->statusLabel->setText(QString("An error occurred while loading the batch scan. Please close and try again."));
    else
    {
        if(d_firstScan > 0)
        {
            ui->analysisWidget->limitRange(d_firstScan,d_lastScan);
		  ui->analysisWidget->loadScan(d_lastScan);
        }
        else
        {
            ui->analysisWidget->limitRange(0,0);
            ui->printScanButton->setEnabled(false);
        }

        ui->statusLabel->setText(QString("Processing complete."));

        ui->analysisWidget->setPlotDelay(ui->afw->delay());
        ui->analysisWidget->setPlotHpf(ui->afw->hpf());
        ui->analysisWidget->setPlotExp(ui->afw->exp());
        ui->analysisWidget->setPlotRdc(ui->afw->removeDC());
        ui->analysisWidget->setPlotPad(ui->afw->zeroPad());
        ui->analysisWidget->setPlotUseWindow(ui->afw->isUseWindow());


        if(p_amdorWidget != nullptr)
        {
            p_amdorWidget->livePlotting(true);
            p_amdorWidget->enableEditing(true);
        }
    }



    batchPlot->enableReplotting();
    unsetCursor();

}
