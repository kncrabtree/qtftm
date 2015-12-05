#include "analysiswidget.h"
#include <QSettings>
#include <QApplication>
#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <qwt6/qwt_plot_renderer.h>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>
#include <QPainter>
#include <QFrame>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include "analysis.h"
#include "autofitwidget.h"

AnalysisWidget::AnalysisWidget(QWidget *parent) :
     QWidget(parent),
     ui(new Ui::AnalysisWidget), llm(new LineListModel(this)), d_currentBaseline(qMakePair(0.0,0.0))
{
	ui->setupUi(this);

	connect(ui->analysisScanSpinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,&AnalysisWidget::loadScan);
	connect(ui->currentScanButton,&QAbstractButton::clicked,this,&AnalysisWidget::showLatestScan);

	connect(llm,&QAbstractItemModel::rowsRemoved,ui->analysisPlot,&FtPlot::replot);
	connect(llm,&LineListModel::modelChanged,this,&AnalysisWidget::linesChanged);

	connect(ui->analysisPlot,&AnalysisPlot::showScanDetails,this,&AnalysisWidget::showScanDetails);
	connect(ui->analysisPlot,&AnalysisPlot::peakMark,this,&AnalysisWidget::peakMarkRequested);
	connect(ui->analysisPlot,&AnalysisPlot::peakMarkCancelled,llm,&LineListModel::deleteDopplerPair);
    connect(ui->analysisPlot,&AnalysisPlot::dopplerFinished,this,&AnalysisWidget::setDopplerAmplitude);
	connect(ui->analysisPlot,&AnalysisPlot::dopplerFinished,this,&AnalysisWidget::saveScanMetaData);
	connect(llm,&LineListModel::dpCreated,ui->analysisPlot,&AnalysisPlot::startPeakMark);
	connect(llm,&LineListModel::showDp,ui->analysisPlot,&AnalysisPlot::showLoadedDopplerPair);
	connect(ui->analysisPlot,&AnalysisPlot::dopplerChanged,llm,&LineListModel::dopplerChanged);
	connect(ui->lineListView,&QWidget::customContextMenuRequested,this,&AnalysisWidget::buildLineListContextMenu);

	connect(ui->analysisPlot,&AnalysisPlot::exportFt,this,&AnalysisWidget::exportFt);
	connect(ui->analysisPlot,&AnalysisPlot::exportFid,this,&AnalysisWidget::exportFid);
	connect(ui->analysisPlot,&AnalysisPlot::autoFitRequested,this,&AnalysisWidget::autoFit);
    connect(ui->analysisPlot,&AnalysisPlot::autoFitDeleteRequested,this,&AnalysisWidget::deleteAutoFit);
	connect(ui->analysisPlot,&AnalysisPlot::fitParametersRequested,this,&AnalysisWidget::viewFitResults);
	connect(ui->analysisPlot,&AnalysisPlot::fitLogRequested,this,&AnalysisWidget::viewFitLog);


	ui->analysisNotes->installEventFilter(this);
	ui->lineListView->setModel(llm);
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	ui->analysisScanSpinBox->setMaximum(s.value(QString("scanNum"),0).toInt());

	fitThread = new QThread(this);
}

AnalysisWidget::~AnalysisWidget()
{
	delete ui;

	fitThread->quit();
	fitThread->wait();
}

FtPlot *AnalysisWidget::plot()
{
    return ui->analysisPlot;
}

void AnalysisWidget::configureForDr()
{
    ui->analysisScanSpinBox->setVisible(false);
    ui->currentScanButton->setVisible(false);
    ui->analysisScanSpinBox->setEnabled(false);
    ui->currentScanButton->setEnabled(false);
    ui->analysisNotes->setVisible(false);
    ui->analysisNotes->setEnabled(false);
    ui->scanLabel->setVisible(false);
    ui->notesLabel->setVisible(false);
    ui->linesLabel->setText(QString("Integration ranges"));
}

void AnalysisWidget::setPlotDelay(const double d)
{
    ui->analysisPlot->setDelay(d);
}

void AnalysisWidget::setPlotHpf(const double h)
{
    ui->analysisPlot->setHpf(h);
}

void AnalysisWidget::setPlotExp(const double e)
{
    ui->analysisPlot->setExp(e);
}

void AnalysisWidget::setPlotRdc(const bool b)
{
    ui->analysisPlot->setRemoveDc(b);
}

void AnalysisWidget::setPlotPad(const bool b)
{
    ui->analysisPlot->setPadding(b);
}

void AnalysisWidget::peakMarkRequested()
{
	llm->newEntry((ui->analysisPlot->xMax()+ui->analysisPlot->xMin())/2.0,ui->analysisPlot->xMin()/4e5);
}

void AnalysisWidget::buildLineListContextMenu(const QPoint p)
{
	QMenu *menu = new QMenu();
	menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction *addAct = menu->addAction(QString("Add to peak list"));
    if(!ui->lineListView->selectionModel()->hasSelection())
        addAct->setEnabled(false);
    connect(addAct,&QAction::triggered,this,&AnalysisWidget::requestPeakAdd);

    menu->addSeparator();

	QAction *dAct = menu->addAction(QString("Delete selected"));
	if(!ui->lineListView->selectionModel()->hasSelection())
		dAct->setEnabled(false);
	connect(dAct,&QAction::triggered,this,&AnalysisWidget::deleteLine);

	menu->addAction(dAct);

	QAction *clearAct = menu->addAction(QString("Clear all lines"));
	if(llm->rowCount()==0)
		clearAct->setEnabled(false);
	connect(clearAct,&QAction::triggered,this,&AnalysisWidget::clearLines);

	menu->popup(ui->lineListView->mapToGlobal(p));
}

void AnalysisWidget::deleteLine()
{
    llm->deleteDopplerPair(ui->lineListView->selectionModel()->currentIndex().row());
	saveScanMetaData();
}

void AnalysisWidget::newScan(Scan s)
{
	if(s.number() < 0)
	{
		//something went wrong with initialization, and no FID was recorded. ignore it
		return;
	}

	if(s.number() > ui->analysisScanSpinBox->maximum())
		ui->analysisScanSpinBox->setMaximum(s.number());

	if( (ui->analysisScanSpinBox->value()==s.number()-1) || ui->analysisScanSpinBox->value()==0)
		showScan(s);
}

void AnalysisWidget::showLatestScan()
{
	ui->analysisScanSpinBox->setValue(ui->analysisScanSpinBox->maximum());
}

void AnalysisWidget::loadScan(int num)
{
	showScan(Scan(num));
}

void AnalysisWidget::showScan(Scan s)
{
	saveScanMetaData();
	d_currentScan = s;
    d_currentBaseline = qMakePair(0.0,0.0);
    if(ui->analysisScanSpinBox->value() != d_currentScan.number())
    {
	    ui->analysisScanSpinBox->blockSignals(true);
	    ui->analysisScanSpinBox->setValue(d_currentScan.number());
	    ui->analysisScanSpinBox->blockSignals(false);
    }
    ui->analysisPlot->detachItems(QwtPlotItem::Rtti_PlotCurve,false);
    ui->analysisPlot->hideTraces();

   if(d_currentScan.number()>0)
    {
	    ui->analysisPlot->newFid(d_currentScan.fid());
	    emit canPrint(true);
	    emit scanChanged(d_currentScan.number());
    }
    else
    {
	    ui->analysisPlot->hideTraces();
	    emit canPrint(false);
    }
   loadScanMetaData();
}

void AnalysisWidget::print()
{
	if(d_currentScan.number() < 1)
		return;

	QPrinter p(QPrinter::HighResolution);
	p.setOrientation(QPrinter::Landscape);
	p.setCreator(QString("QtFTM"));
	QPrintDialog pd(&p);

	if(pd.exec())
	{

		QFrame *df = renderPrintSidebar();

		QPainter painter;
		painter.begin(&p);

		double xscale = ((double)p.pageRect().width())/4.0/(double)df->width();
		double yscale = (double)p.pageRect().height()/(double)df->height();
		double scale = qMin(xscale, yscale);
		painter.scale(scale,scale);

		df->render(&painter,QPoint(),QRegion(),DrawChildren);

		painter.scale(1.0/scale,1.0/scale);
		ui->analysisPlot->renderTo(&painter,QRect(p.width()/4,0,3*p.width()/4,p.height()));

		painter.end();

		delete df;

	}

}

void AnalysisWidget::clearLines()
{
	llm->deleteAll();
	ui->analysisPlot->clearDopplerPairs();
	saveScanMetaData();
}

void AnalysisWidget::showScanDetails()
{
	if(!d_currentScan.isSaved())
		return;

	QDialog *d = new QDialog(this);
	d->setWindowTitle(QString("Scan %1 details").arg(d_currentScan.number()));
	QFrame *df = renderPrintSidebar();
	df->setPalette(QPalette());
	df->setParent(d);
	QVBoxLayout *vl = new QVBoxLayout(d);
	vl->addWidget(df);
	QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Close,Qt::Horizontal,d);
	connect(bbox,&QDialogButtonBox::clicked,d,&QDialog::accept);
	vl->addWidget(bbox);
	d->setLayout(vl);

	connect(d,&QDialog::finished,d,&QObject::deleteLater);
    d->show();
}

void AnalysisWidget::enableSelection(bool enabled)
{
    ui->currentScanButton->setEnabled(enabled);
    ui->analysisScanSpinBox->setEnabled(enabled);

}

void AnalysisWidget::limitRange(int first, int last)
{
    ui->analysisScanSpinBox->setRange(first,last);
    if(first>0)
        ui->analysisScanSpinBox->setSpecialValueText(QString(""));
    else
	    ui->analysisScanSpinBox->setSpecialValueText(QString("None"));
}

void AnalysisWidget::saveScanMetaData()
{
	if(d_currentScan.number() <= 0)
		return;


	int dirMillionsNum = (int)floor((double) d_currentScan.number()/1000000.0);
	int dirThousandsNum = (int)floor((double) d_currentScan.number()/1000.0);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QString savePath = s.value(QString("savePath"),QString(".")).toString();

	QDir d(savePath + QString("/metadata/%1/%2").arg(dirMillionsNum).arg(dirThousandsNum));
	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
			return;
	}

	//create output file
	QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(d_currentScan.number()));

	if(ui->analysisNotes->document()->isEmpty() && llm->rowCount() == 0)
	{
		if(!f.exists())
			return;
	}

	if(!f.open(QIODevice::WriteOnly))
		return;

	QStringList lines = llm->getMetaData();
	for(int i=0;i<lines.size();i++)
		f.write(lines.at(i).toLatin1());
	if(!ui->analysisNotes->document()->isEmpty())
	{
		f.write(QByteArray("#Notes\n"));
		f.write(ui->analysisNotes->document()->toPlainText().toLatin1());
	}

	f.close();
	emit metaDataChanged(d_currentScan.number());
}

void AnalysisWidget::loadScanMetaData()
{
	llm->deleteAll();
	ui->analysisPlot->clearDopplerPairs();
	ui->analysisNotes->clear();

	if(d_currentScan.number() <= 0)
		return;

	int dirMillionsNum = (int)floor((double) d_currentScan.number()/1000000.0);
	int dirThousandsNum = (int)floor((double) d_currentScan.number()/1000.0);

    //get fit result (if exists)
    FitResult res(d_currentScan.number());
    d_currentBaseline = res.baselineY0Slope();

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString savePath = s.value(QString("savePath"),QString(".")).toString();

	//open metadata file
	QFile f(savePath + QString("/metadata/%1/%2/%3.txt").arg(dirMillionsNum).arg(dirThousandsNum).arg(d_currentScan.number()));
	if(f.exists() && f.open(QIODevice::ReadOnly))
	{
		while(!f.atEnd())
		{
			QByteArray line = f.readLine().trimmed();
			if(line.startsWith(QByteArray("#Notes")))
				break;

			//if we made it here, this should be a doppler pair
			QStringList l = QString(line).split(QString("\t"),QString::SkipEmptyParts);
            if(l.size()<2)
				continue;

			bool ok = false;
			double c = l.at(0).toDouble(&ok);
			if(!ok)
				continue;

			double s = l.at(1).toDouble(&ok);
			if(!ok)
				continue;

            double a = 0.0;
            if(l.size() > 2)
                a = l.at(2).toDouble();

            llm->addLine(c,s,a);
		}

		if(!f.atEnd())
			ui->analysisNotes->appendPlainText(QString(f.readAll()));

		f.close();
	}


	if(res.category() == FitResult::Invalid || res.category() == FitResult::Saturated)
	{
        if(res.category() == FitResult::Invalid && !res.log().isEmpty())
        {
            if(!ui->analysisNotes->document()->toPlainText().contains(QString("AutoFit failed.")))
                ui->analysisNotes->appendPlainText(QString("AutoFit failed."));
        }
        else if(res.category() == FitResult::Saturated)
        {
            if(!ui->analysisNotes->document()->toPlainText().contains(QString("FID saturated.")))
                ui->analysisNotes->appendPlainText(QString("FID saturated."));
        }
		ui->analysisPlot->newFit(QVector<QPointF>());
		return;
	}

    setPlotDelay(res.delay());
    setPlotHpf(res.hpf());
    setPlotExp(res.exp());

	if(res.category() == FitResult::Success)
	{
		if(res.type() == FitResult::LorentzianSingleLMS || res.type() == FitResult::LorentzianSingleLM)
		{
			for(int i=0;i<res.freqAmpSingleList().size();i++)
                llm->addUniqueLine(res.freqAmpSingleList().at(i).first,0.0,res.freqAmpSingleList().at(i).second);
		}
		else
		{
			if(res.allFitParams().size()>3)
			{
				double split = res.allFitParams().at(2);
				for(int i=0;i<res.freqAmpPairList().size();i++)
					llm->addUniqueLine(res.freqAmpPairList().at(i).first,split/2.0,res.freqAmpPairList().at(i).second);
				for(int i=0;i<res.freqAmpSingleList().size();i++)
					llm->addUniqueLine(res.freqAmpSingleList().at(i).first,0.0,res.freqAmpSingleList().at(i).second);
			}
		}

	}

	ui->analysisPlot->newFit(res.toXY());

}

void AnalysisWidget::checkForLoadScanMetaData(int scanNum)
{
	if(scanNum == d_currentScan.number())
		loadScanMetaData();
}

void AnalysisWidget::exportFt()
{
	if(d_currentScan.number() < 0)
		return;

	QString name = QFileDialog::getSaveFileName(this,QString("Export FT"),QString("~/%1_ft.txt").arg(d_currentScan.number()));
	if(name.isEmpty())
		return;

	QFile f(name);

	if(!f.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this,QString("FT Export Failed"),QString("Could not open file %1 for writing. Please choose a different filename.").arg(name));
		return;
	}

	f.write(QString("freq%1\tft%1").arg(d_currentScan.number()).toLatin1());
	QVector<QPointF> ft = ui->analysisPlot->getFt();
	for(int i=0;i<ft.size();i++)
		f.write(QString("\n%1\t%2").arg(ft.at(i).x(),0,'f',4).arg(ft.at(i).y(),0,'e',9).toLatin1());
	f.close();
}

void AnalysisWidget::exportFid()
{
	if(d_currentScan.number() < 0)
		return;

	QString name = QFileDialog::getSaveFileName(this,QString("Export FID"),QString("~/%1_fid.txt").arg(d_currentScan.number()));
	if(name.isEmpty())
		return;

	QFile f(name);

	if(!f.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this,QString("FID Export Failed"),QString("Could not open file %1 for writing. Please choose a different filename.").arg(name));
		return;
	}

	f.write(QString("time%1\tfid%1").arg(d_currentScan.number()).toLatin1());
	QVector<QPointF> fid = ui->analysisPlot->getFid();
	for(int i=0;i<fid.size();i++)
		f.write(QString("\n%1\t%2").arg(fid.at(i).x()/1e6,0,'e',4).arg(fid.at(i).y(),0,'e',9).toLatin1());
	f.close();
}

void AnalysisWidget::autoFit()
{
	if(fitThread->isRunning())
		return;

	FitResult res(d_currentScan.number());
	FitResult::BufferGas bg = Analysis::bufferNe;

	//guess buffer gas from scan?
	QStringList gasList = d_currentScan.gasNames();
	QList<double> flows = d_currentScan.gasFlows();
	QString bgName;
	double max = 0.0;
	for(int i=0;i<flows.size() && i < gasList.size(); i++)
	{
		if(flows.at(i) > max)
		{
			max = flows.at(i);
			bgName = gasList.at(i);
		}
	}

	if(!bgName.isEmpty())
		bg = Analysis::bgFromString(bgName);

	double t = 293.15;
	if(res.category() != FitResult::Invalid)
	{
		bg = res.bufferGas();
		t = res.temperature();
	}

	double delay = ui->analysisPlot->getDelay();
	double hpf = ui->analysisPlot->getHpf();
	double exp = ui->analysisPlot->getExp();
	bool pad = ui->analysisPlot->getPadFidBox()->isChecked();

	QDialog d;
	d.setWindowTitle(QString("AutoFit Settings"));
	AutoFitWidget *aw = new AutoFitWidget(bg.name,delay,hpf,exp,pad,t,&d);
	aw->setNoDisable();
	QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	QVBoxLayout *vl = new QVBoxLayout(&d);
	connect(bb->button(QDialogButtonBox::Ok),&QAbstractButton::clicked,&d,&QDialog::accept);
	connect(bb->button(QDialogButtonBox::Cancel),&QAbstractButton::clicked,&d,&QDialog::reject);
	vl->addWidget(aw);
	vl->addWidget(bb);
	d.setLayout(vl);

	if(d.exec() == QDialog::Rejected)
		return;

	AbstractFitter *fitter = aw->toFitter();
    connect(fitter,&AbstractFitter::fitComplete,this,&AnalysisWidget::autoFitComplete);
	connect(fitter,&AbstractFitter::fitComplete,fitThread,&QThread::quit);
	connect(fitThread,&QThread::finished,fitter,&QObject::deleteLater);
	fitter->moveToThread(fitThread);
	fitThread->start();
	QMetaObject::invokeMethod(fitter,"doFit",Q_ARG(const Scan,d_currentScan));
	QApplication::setOverrideCursor(Qt::BusyCursor);
}

void AnalysisWidget::autoFitComplete(const FitResult &res)
{
    Q_UNUSED(res)
	QApplication::restoreOverrideCursor();
    loadScanMetaData();
}

void AnalysisWidget::deleteAutoFit()
{
    if(fitThread->isRunning())
        return;

    FitResult::deleteFitResult(d_currentScan.number());
    loadScanMetaData();
}

void AnalysisWidget::viewFitResults()
{
	if(fitThread->isRunning())
		return;

	FitResult res(d_currentScan.number());
	if(res.category() == FitResult::Invalid)
	{
		QMessageBox::information(this,QString("No Fit Available"),QString("No fit has been performed. Please run an AutoFit to see the fit parameters."),QMessageBox::Ok);
		return;
	}
	else if(res.category() == FitResult::Saturated)
	{
		QMessageBox::information(this,QString("FID Saturated"),QString("AutoFit has determined that the FID is saturated."),QMessageBox::Ok);
		return;
	}

	QStringList p = res.parameterList();
	QString msg;
	for(int i=0; i<p.size(); i++)
		msg.append(p.at(i)).append(QString("\n"));
	QMessageBox::information(this,QString("AutoFit Parameters: Scan %1").arg(d_currentScan.number()),msg,QMessageBox::Ok);

}

void AnalysisWidget::viewFitLog()
{
	if(fitThread->isRunning())
		return;

	FitResult res(d_currentScan.number());
	if(res.category() == FitResult::Invalid)
	{
		QMessageBox::information(this,QString("No Fit Available"),QString("No fit has been performed. Please run an AutoFit to see the fit log."),QMessageBox::Ok);
		return;
	}

	QDialog d;
	d.setWindowTitle(QString("Fit log for scan %1").arg(d_currentScan.number()));
	QVBoxLayout *vl = new QVBoxLayout();
	QPlainTextEdit *te = new QPlainTextEdit(res.log(),&d);
	te->setReadOnly(true);
	te->setMinimumWidth(500);
	te->setMinimumHeight(350);
	QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok,Qt::Horizontal,&d);
	connect(bb->button(QDialogButtonBox::Ok),&QAbstractButton::clicked,&d,&QDialog::accept);
	vl->addWidget(te);
	vl->addWidget(bb);
	d.setLayout(vl);
    d.exec();
}

void AnalysisWidget::setDopplerAmplitude(DopplerPair *d)
{
    Analysis::estimateDopplerPairAmplitude(ui->analysisPlot->currentFtXY(),d,d_currentBaseline);
}

void AnalysisWidget::requestPeakAdd()
{
    int index = ui->lineListView->selectionModel()->currentIndex().row();
    DopplerPair *dp = llm->getLine(index);
    if(dp)
    {
        emit peakAddRequested(d_currentScan.number(),dp->center(),dp->amplitude());
    }
}

bool AnalysisWidget::eventFilter(QObject *obj, QEvent *ev)
{
	if(ev->type() == QEvent::FocusOut)
	{
		if(obj == ui->analysisNotes)
			saveScanMetaData();
	}

	return QWidget::eventFilter(obj,ev);
}

QFrame *AnalysisWidget::renderPrintSidebar()
{
	QFrame *df = new QFrame();
	QPalette pal;
    int lnum = 0;
	pal.setColor(QPalette::Base,QColor(Qt::white));
	pal.setColor(QPalette::AlternateBase,QColor(Qt::white));
	pal.setColor(QPalette::Text,QColor(Qt::black));
	pal.setColor(QPalette::Window,QColor(Qt::white));
	pal.setColor(QPalette::WindowText,QColor(Qt::black));
	df->setPalette(pal);

	df->setFrameStyle(QFrame::Box | QFrame::Plain);
	df->setLineWidth(1);
	df->setGeometry(0,0,1100/4,850);

	QVBoxLayout *vbl = new QVBoxLayout;

	QGridLayout *sl = new QGridLayout;

	QLabel *dateText = new QLabel(QString("<b>%1</b>").arg(d_currentScan.timeStamp().toString()));
	dateText->setAlignment(Qt::AlignCenter);
    sl->addWidget(dateText,lnum++,0,1,3,Qt::AlignCenter);


	QLabel *scanText = new QLabel(QString("Scan"));
	scanText->setAlignment(Qt::AlignLeft);
    sl->addWidget(scanText,lnum,0,Qt::AlignLeft);

	QLabel *scanVal = new QLabel(QString::number(d_currentScan.number()));
	scanVal->setAlignment(Qt::AlignRight);
    sl->addWidget(scanVal,lnum,1,Qt::AlignRight);

    QLabel *scanUnit = new QLabel(QString("FTM%1").arg(QTFTM_SPECTROMETER));
	scanUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(scanUnit,lnum++,2,Qt::AlignLeft);

	QLabel *shotsText = new QLabel(QString("Shots"));
	shotsText->setAlignment(Qt::AlignLeft);
    sl->addWidget(shotsText,lnum,0,Qt::AlignLeft);

	QLabel *shotsVal = new QLabel(QString::number(d_currentScan.completedShots()));
	shotsVal->setAlignment(Qt::AlignRight);
    sl->addWidget(shotsVal,lnum,1,Qt::AlignRight);

	QLabel *shotsUnit = new QLabel();
	shotsUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(scanUnit,lnum++,2,Qt::AlignLeft);

    QLabel *cFreqText = new QLabel(QString("Cavity frequency"));
	cFreqText->setAlignment(Qt::AlignLeft);
    sl->addWidget(cFreqText,lnum,0,Qt::AlignLeft);

	QLabel *cFreqVal = new QLabel(QString::number(d_currentScan.ftFreq(),'f',4));
	cFreqVal->setAlignment(Qt::AlignRight);
    sl->addWidget(cFreqVal,lnum,1,Qt::AlignRight);

	QLabel *cFreqUnit = new QLabel(QString("MHz"));
	cFreqUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(cFreqUnit,lnum++,2,Qt::AlignLeft);

    QLabel *skipTuneText = new QLabel(QString("Skipped Tuning?"));
	skipTuneText->setAlignment(Qt::AlignLeft);
    sl->addWidget(skipTuneText,lnum,0,Qt::AlignLeft);

	QLabel *skipTuneVal = new QLabel();
	if(d_currentScan.skipTune())
		skipTuneVal->setText(QString("Yes"));
	else
		skipTuneVal->setText(QString("No"));
	skipTuneVal->setAlignment(Qt::AlignRight);
    sl->addWidget(skipTuneVal,lnum,1,Qt::AlignRight);

	QLabel *skipTuneUnit = new QLabel(QString(""));
	skipTuneUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(skipTuneUnit,lnum++,2,Qt::AlignLeft);

    QLabel *tVoltText = new QLabel(QString("Tuning Voltage"));
    tVoltText->setAlignment(Qt::AlignLeft);
    sl->addWidget(tVoltText,lnum,0,Qt::AlignLeft);

    QLabel *tVoltVal = new QLabel(QString(QString::number(d_currentScan.tuningVoltage())));
    tVoltVal->setAlignment(Qt::AlignRight);
    sl->addWidget(tVoltVal,lnum,1,Qt::AlignRight);

    QLabel *tVoltUnit = new QLabel(QString("mV"));
    tVoltUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(tVoltUnit,lnum++,2,Qt::AlignLeft);

	QLabel *attenText = new QLabel(QString("Attenuation"));
	attenText->setAlignment(Qt::AlignLeft);
    sl->addWidget(attenText,lnum,0,Qt::AlignLeft);

	QLabel *attenVal = new QLabel(QString::number(d_currentScan.attenuation()));
	attenVal->setAlignment(Qt::AlignRight);
    sl->addWidget(attenVal,lnum,1,Qt::AlignRight);

    QLabel *attenUnit = new QLabel(QString("dB"));
    attenUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(attenUnit,lnum++,2,Qt::AlignLeft);


    QLabel *cVoltText = new QLabel(QString("Cavity Voltage"));
    cVoltText->setAlignment(Qt::AlignLeft);
    sl->addWidget(cVoltText,lnum,0,Qt::AlignLeft);

    QLabel *cVoltVal = new QLabel(QString::number(d_currentScan.cavityVoltage()));
    cVoltVal->setAlignment(Qt::AlignRight);
    sl->addWidget(cVoltVal,lnum,1,Qt::AlignRight);

    QLabel *cVoltUnit = new QLabel(QString("mV"));
    cVoltUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(cVoltUnit,lnum++,2,Qt::AlignLeft);


    QLabel *cDipoleText = new QLabel(QString("Target Dipole"));
    cDipoleText->setAlignment(Qt::AlignLeft);
    sl->addWidget(cDipoleText,lnum,0,Qt::AlignLeft);

    QLabel *cDipoleVal = new QLabel(QString::number(d_currentScan.dipoleMoment()));
    cDipoleVal->setAlignment(Qt::AlignRight);
    sl->addWidget(cDipoleVal,lnum,1,Qt::AlignRight);

    QLabel *cDipoleUnit = new QLabel(QString("D"));
    cDipoleUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(cDipoleUnit,lnum++,2,Qt::AlignLeft);




    QLabel *pDelayText = new QLabel(QString("Protection Delay"));
    pDelayText->setAlignment(Qt::AlignLeft);
    sl->addWidget(pDelayText,lnum,0,Qt::AlignLeft);

    QLabel *pDelayVal = new QLabel(QString::number(d_currentScan.protectionDelayTime()));
    pDelayVal->setAlignment(Qt::AlignRight);
    sl->addWidget(pDelayVal,lnum,1,Qt::AlignRight);

    QLabel *pDelayUnit = new QLabel(QString::fromUtf8(" μs"));
    pDelayUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(pDelayUnit,lnum++,2,Qt::AlignLeft);


    QLabel *sDelayText = new QLabel(QString("Scope Delay"));
    sDelayText->setAlignment(Qt::AlignLeft);
    sl->addWidget(sDelayText,lnum,0,Qt::AlignLeft);

    QLabel *sDelayVal = new QLabel(QString::number(d_currentScan.scopeDelayTime()));
    sDelayVal->setAlignment(Qt::AlignRight);
    sl->addWidget(sDelayVal,lnum,1,Qt::AlignRight);

    QLabel *sDelayUnit = new QLabel(QString::fromUtf8(" μs"));
    sDelayUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(sDelayUnit,lnum++,2,Qt::AlignLeft);

	QLabel *drFreqText = new QLabel(QString("DR frequency"));
	drFreqText->setAlignment(Qt::AlignLeft);
    sl->addWidget(drFreqText,lnum,0,Qt::AlignLeft);

	QLabel *drFreqVal = new QLabel(QString::number(d_currentScan.drFreq(),'f',4));
	drFreqVal->setAlignment(Qt::AlignRight);
    sl->addWidget(drFreqVal,lnum,1,Qt::AlignRight);

	QLabel *drFreqUnit = new QLabel(QString("MHz"));
	drFreqUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(drFreqUnit,lnum++,2,Qt::AlignLeft);

	QLabel *drPwrText = new QLabel(QString("DR power"));
	drPwrText->setAlignment(Qt::AlignLeft);
    sl->addWidget(drPwrText,lnum,0,Qt::AlignLeft);

	QLabel *drPwrVal = new QLabel(QString::number(d_currentScan.drPower(),'f',1));
	drPwrVal->setAlignment(Qt::AlignRight);
    sl->addWidget(drPwrVal,lnum,1,Qt::AlignRight);

	QLabel *drPwrUnit = new QLabel(QString("dBm"));
	drPwrUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(drPwrUnit,lnum++,2,Qt::AlignLeft);

    QLabel *dcvText = new QLabel(QString("DC voltage"));
    dcvText->setAlignment(Qt::AlignLeft);
   sl->addWidget(dcvText,lnum,0,Qt::AlignLeft);

    QLabel *dcvVal = new QLabel(QString::number(d_currentScan.dcVoltage()));
    dcvVal->setAlignment(Qt::AlignRight);
   sl->addWidget(dcvVal,lnum,1,Qt::AlignRight);

    QLabel *dcvUnit = new QLabel(QString("V"));
    dcvUnit->setAlignment(Qt::AlignLeft);
   sl->addWidget(dcvUnit,lnum++,2,Qt::AlignLeft);

	QLabel *probeFreqText = new QLabel(QString("Probe frequency"));
	probeFreqText->setAlignment(Qt::AlignLeft);
    sl->addWidget(probeFreqText,lnum,0,Qt::AlignLeft);

	QLabel *probeFreqVal = new QLabel(QString::number(d_currentScan.fid().probeFreq(),'f',4));
	probeFreqVal->setAlignment(Qt::AlignRight);
    sl->addWidget(probeFreqVal,lnum,1,Qt::AlignRight);

	QLabel *probeFreqUnit = new QLabel(QString("MHz"));
	probeFreqUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(probeFreqUnit,lnum++,2,Qt::AlignLeft);

	QLabel *resText = new QLabel(QString("Resolution"));
	resText->setAlignment(Qt::AlignLeft);
    sl->addWidget(resText,lnum,0,Qt::AlignLeft);

	double resolution = 1.0/(d_currentScan.fid().spacing()*d_currentScan.fid().size())/1000.0;
	QLabel *resVal = new QLabel(QString::number(resolution,'f',4));
	resVal->setAlignment(Qt::AlignRight);
    sl->addWidget(resVal,lnum,1,Qt::AlignRight);

	QLabel *resUnit = new QLabel(QString("kHz"));
	resUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(resUnit,lnum++,2,Qt::AlignLeft);

	QLabel *truncText = new QLabel(QString("Truncation"));
	truncText->setAlignment(Qt::AlignLeft);
    sl->addWidget(truncText,lnum,0,Qt::AlignLeft);

	QLabel *truncVal = new QLabel(QString::number(ui->analysisPlot->getDelay(),'f',1));
	truncVal->setAlignment(Qt::AlignRight);
    sl->addWidget(truncVal,lnum,1,Qt::AlignRight);

	QLabel *truncUnit = new QLabel(QString::fromUtf8(" μs"));
	truncUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(truncUnit,lnum++,2,Qt::AlignLeft);

	QLabel *hpfText = new QLabel(QString("High pass filter"));
	hpfText->setAlignment(Qt::AlignLeft);
    sl->addWidget(hpfText,lnum,0,Qt::AlignLeft);

	QLabel *hpfVal = new QLabel(QString::number(ui->analysisPlot->getHpf(),'f',0));
	hpfVal->setAlignment(Qt::AlignRight);
    sl->addWidget(hpfVal,lnum,1,Qt::AlignRight);

	QLabel *hpfUnit = new QLabel(QString(" kHz"));
	hpfUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(hpfUnit,lnum++,2,Qt::AlignLeft);

	QLabel *expText = new QLabel(QString("Exponential filter"));
	expText->setAlignment(Qt::AlignLeft);
    sl->addWidget(expText,lnum,0,Qt::AlignLeft);

	QLabel *expVal = new QLabel(QString::number(ui->analysisPlot->getExp(),'f',1));
	expVal->setAlignment(Qt::AlignRight);
    sl->addWidget(expVal,lnum,1,Qt::AlignRight);

	QLabel *expUnit = new QLabel(QString::fromUtf8(" μs"));
	expUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(expUnit,lnum++,2,Qt::AlignLeft);

	QLabel *pressureText = new QLabel(QString("Pressure"));
	pressureText->setAlignment(Qt::AlignLeft);
    sl->addWidget(pressureText,lnum,0,Qt::AlignLeft);

	QLabel *pressureVal = new QLabel(QString::number(d_currentScan.pressure(),'f',3));
    pressureVal->setAlignment(Qt::AlignRight);
    sl->addWidget(pressureVal,lnum,1,Qt::AlignRight);

	QLabel *pressureUnit = new QLabel(QString("kTorr"));
	pressureUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(pressureUnit,lnum++,2,Qt::AlignLeft);

    QLabel *repRateText = new QLabel(QString("Rep rate"));
    repRateText->setAlignment(Qt::AlignLeft);
    sl->addWidget(repRateText,lnum,0,Qt::AlignLeft);

    QLabel *repRateVal = new QLabel(QString::number(d_currentScan.repRate(),'f',1));
    repRateVal->setAlignment(Qt::AlignRight);
    sl->addWidget(repRateVal,lnum,1,Qt::AlignRight);

    QLabel *repRateUnit = new QLabel(QString("Hz"));
    repRateUnit->setAlignment(Qt::AlignLeft);
    sl->addWidget(repRateUnit,lnum++,2,Qt::AlignLeft);

    QLabel *magnetText = new QLabel(QString("Magnet enabled?"));
    magnetText->setAlignment(Qt::AlignLeft);
    sl->addWidget(magnetText,lnum,0,Qt::AlignLeft);

    QLabel *magnetVal = new QLabel();
    if(d_currentScan.magnet())
	    magnetVal->setText(QString("Yes"));
    else
	    magnetVal->setText(QString("No"));
    magnetVal->setAlignment(Qt::AlignRight);
    sl->addWidget(magnetVal,lnum,1,Qt::AlignRight);

    QLabel *magnetUnit = new QLabel(QString(""));
    sl->addWidget(magnetUnit,lnum++,2);


	sl->setColumnStretch(0,1);
	sl->setColumnStretch(1,0);
	sl->setColumnStretch(2,0);
	vbl->addLayout(sl);

	QGridLayout *gl = new QGridLayout;

	QLabel *gasLabel = new QLabel(QString("<b>Gas Flow Rates (sccm)<b>"));
	gasLabel->setAlignment(Qt::AlignHCenter);
	gl->addWidget(gasLabel,0,0,1,3);

	for(int i=0; i<d_currentScan.gasFlows().size(); i++)
	{
		QLabel *gt = new QLabel(QString("Ch %1: ").arg(QString::number(i+1)));
		gt->setAlignment(Qt::AlignLeft);
		gl->addWidget(gt,i+1,0,Qt::AlignLeft);

		QLabel *gn = new QLabel;
		if(i<d_currentScan.gasNames().size())
			gn->setText(d_currentScan.gasNames().at(i));
		else
			gn->setText(QString(""));
		gn->setAlignment(Qt::AlignLeft);
		gl->addWidget(gn,i+1,1,Qt::AlignLeft);

		QLabel *gv = new QLabel(QString::number(d_currentScan.gasFlows().at(i),'f',3));
		gv->setAlignment(Qt::AlignRight);
		gl->addWidget(gv,i+1,2,Qt::AlignRight);
	}

	gl->setColumnStretch(0,0);
	gl->setColumnStretch(1,1);
	gl->setColumnStretch(2,0);


	vbl->addLayout(gl);

	QGridLayout *pl = new QGridLayout;

	QLabel *pulseTitle = new QLabel(QString("<b>Pulse Configuration</b>"));
	pulseTitle->setAlignment(Qt::AlignHCenter);
	pl->addWidget(pulseTitle,0,0,1,5,Qt::AlignCenter);

	QLabel *pcTitle = new QLabel(QString("Ch"));
	pcTitle->setAlignment(Qt::AlignLeft);
	pl->addWidget(pcTitle,1,0,Qt::AlignLeft);

	QLabel *pdTitle = new QLabel(QString("Delay (<span>&mu;</span>s)"));
	pdTitle->setAlignment(Qt::AlignHCenter);
	pl->addWidget(pdTitle,1,1,Qt::AlignCenter);

	QLabel *pwTitle = new QLabel(QString("Width (<span>&mu;</span>s)"));
	pwTitle->setAlignment(Qt::AlignHCenter);
	pl->addWidget(pwTitle,1,2,Qt::AlignCenter);

	QLabel *paTitle = new QLabel(QString("Active"));
	paTitle->setAlignment(Qt::AlignHCenter);
	pl->addWidget(paTitle,1,3,Qt::AlignCenter);

	QLabel *peTitle = new QLabel(QString("On"));
	peTitle->setAlignment(Qt::AlignHCenter);
	pl->addWidget(peTitle,1,4,Qt::AlignCenter);

	for(int i=0;i<d_currentScan.pulseConfiguration().size();i++)
	{
		QLabel *pn = new QLabel(d_currentScan.pulseConfiguration().at(i).channelName);
		pn->setAlignment(Qt::AlignLeft);
		pl->addWidget(pn,i+2,0,Qt::AlignLeft);

		QLabel *pd = new QLabel(QString::number(d_currentScan.pulseConfiguration().at(i).delay,'f',1));
		pd->setAlignment(Qt::AlignCenter);
		pl->addWidget(pd,i+2,1,Qt::AlignCenter);

		QLabel *pw = new QLabel(QString::number(d_currentScan.pulseConfiguration().at(i).width,'f',1));
		pw->setAlignment(Qt::AlignCenter);
		pl->addWidget(pw,i+2,2,Qt::AlignCenter);

		QLabel *pa = new QLabel;
		QtFTM::PulseActiveLevel al = d_currentScan.pulseConfiguration().at(i).level;
		if(al == QtFTM::PulseLevelActiveHigh)
			pa->setText(QString("High"));
		else
			pa->setText(QString("Low"));
		pa->setAlignment(Qt::AlignCenter);
		pl->addWidget(pa,i+2,3,Qt::AlignCenter);

		QLabel *pe = new QLabel;
		bool on = d_currentScan.pulseConfiguration().at(i).enabled;
		if(on)
			pe->setText(QString("Yes"));
		else
			pe->setText(QString("No"));
		pe->setAlignment(Qt::AlignCenter);
		pl->addWidget(pe,i+2,4,Qt::AlignCenter);
	}

	pl->setColumnStretch(0,0);
	pl->setColumnStretch(1,1);
	pl->setColumnStretch(2,1);
	pl->setColumnStretch(3,0);
	pl->setColumnStretch(4,0);

	vbl->addLayout(pl);

	QStringList lines = llm->getAllLines();

	if(!lines.isEmpty())
	{
		QVBoxLayout *lvbl = new QVBoxLayout;

		QLabel *lineTitle = new QLabel("<b>Lines</b>");
		lineTitle->setAlignment(Qt::AlignCenter);
		lvbl->addWidget(lineTitle);

		for(int i=0;i<lines.size();i++)
		{
			QLabel *line = new QLabel(lines.at(i));
			line->setAlignment(Qt::AlignLeft);
			lvbl->addWidget(line);
		}

		vbl->addLayout(lvbl);
	}

	if(!ui->analysisNotes->document()->isEmpty())
	{
		QLabel *notesTitle = new QLabel(QString("<b>Notes</b>"));
		notesTitle->setAlignment(Qt::AlignCenter);
		vbl->addWidget(notesTitle,0,Qt::AlignCenter);

        QLabel *notesText = new QLabel(ui->analysisNotes->toPlainText());
		notesText->setWordWrap(true);
		notesText->setAlignment(Qt::AlignLeft);
		vbl->addWidget(notesText,0,Qt::AlignLeft);
	}

	vbl->addSpacerItem(new QSpacerItem(10,0,QSizePolicy::Minimum,QSizePolicy::Expanding));


	df->setLayout(vbl);

	return df;
}
