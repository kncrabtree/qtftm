#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QSettings>
#include <QGraphicsObject>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QWidgetAction>
#include <QInputDialog>
#include <QDateTime>

#include "scan.h"
#include "singlescandialog.h"
#include "singlescanwidget.h"
#include "batchsingle.h"
#include "batchsurvey.h"
#include "batchdr.h"
#include "amdorbatch.h"
#include "batch.h"
#include "batchwizard.h"
#include "communicationdialog.h"
#include "settingsdialog.h"
#include "ftsynthsettingswidget.h"
#include "drsynthsettingswidget.h"
#include "ioboardconfigdialog.h"
#include "loadbatchdialog.h"
#include "batchviewwidget.h"
#include "batchattenuation.h"
#include "dopplerpairfitter.h"
#include "amdorwidget.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), d_hardwareConnected(false), d_logCount(0), d_logIcon(QtFTM::LogNormal)
{

	//build UI and make trivial connections
	ui->setupUi(this);

    p_amdorWidget = nullptr;

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	ui->rollingAvgsSpinBox->setValue(s.value(QString("peakUpAvgs"),20).toInt());
    ui->activateLogOnErrorBox->setChecked(s.value(QString("activateLogOnError"),true).toBool());

    p_sleepTimer = new QTimer(this);
    p_sleepTimer->setSingleShot(true);
    connect(p_sleepTimer,&QTimer::timeout,ui->actionSleep_Mode,&QAction::trigger);

    QActionGroup *resGroup = new QActionGroup(ui->menuResolution);
    resGroup->setExclusive(true);

    res1kHzAction = resGroup->addAction(QString("1 kHz"));
    res1kHzAction->setCheckable(true);

    res2kHzAction = resGroup->addAction(QString("2 kHz"));
    res2kHzAction->setCheckable(true);

    res5kHzAction = resGroup->addAction(QString("5 kHz"));
    res5kHzAction->setCheckable(true);

    res10kHzAction = resGroup->addAction(QString("10 kHz"));
    res10kHzAction->setCheckable(true);


    QtFTM::ScopeResolution r = (QtFTM::ScopeResolution)s.value(QString("scope/%1/resolution")
												   .arg(s.value(QString("scope/subKey"),QString("virtual")).toString()),
												   (int)QtFTM::Res_5kHz).toInt();

    switch(r)
    {
    case QtFTM::Res_1kHz:
        res1kHzAction->setChecked(true);
        break;
    case QtFTM::Res_2kHz:
        res2kHzAction->setChecked(true);
        break;
    case QtFTM::Res_5kHz:
        res5kHzAction->setChecked(true);
        break;
    case QtFTM::Res_10kHz:
        res10kHzAction->setChecked(true);
        break;
    }

    ui->menuResolution->addActions(resGroup->actions());

    QGridLayout *gl = new QGridLayout;
    for(int i=0; i<QTFTM_PGEN_NUMCHANNELS; i++)
    {
        QLabel *lbl = new QLabel(QString("Ch%1").arg(i),this);
        lbl->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        Led *led = new Led(this);
        gl->addWidget(lbl,i/2,(2*i)%4,1,1,Qt::AlignVCenter);
        gl->addWidget(led,i/2,((2*i)%4)+1,1,1,Qt::AlignVCenter);

        d_ledList.append(qMakePair(lbl,led));
    }
    gl->setColumnStretch(0,1);
    gl->setColumnStretch(1,0);
    gl->setColumnStretch(2,1);
    gl->setColumnStretch(3,0);
    ui->pulseConfigBox->setLayout(gl);

    auto doubleVc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    auto intVc = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);

	connect(ui->actionPause,&QAction::triggered,this,&MainWindow::pauseAcq);
	connect(ui->actionResume,&QAction::triggered,this,&MainWindow::resumeAcq);
	connect(ui->actionPrint_Scan,&QAction::triggered,ui->analysisWidget,&AnalysisWidget::print);
	connect(ui->actionSleep_Mode,&QAction::triggered,this,&MainWindow::sleep);
    connect(ui->actionDelayed_Sleep,&QAction::triggered,this,&MainWindow::delayedSleep);
	connect(ui->actionCommunication,&QAction::triggered,this,&MainWindow::launchCommunicationDialog);
	connect(ui->actionFT_Synth,&QAction::triggered,this,&MainWindow::launchFtSettings);
	connect(ui->actionDR_Synth,&QAction::triggered,this,&MainWindow::launchDrSettings);
    connect(ui->actionIO_Board,&QAction::triggered,this,&MainWindow::launchIOBoardSettings);
    connect(ui->actionView_Batch,&QAction::triggered,this,&MainWindow::viewBatchCallback);
    connect(ui->actionChange_Tuning_File,&QAction::triggered,this,&MainWindow::changeAttnFileCallback);
    connect(ui->actionGenerate_Tuning_Table,&QAction::triggered,this,&MainWindow::genAttnTableCallback);
	connect(ui->analysisWidget,&AnalysisWidget::canPrint,ui->actionPrint_Scan,&QAction::setEnabled);
    connect(ui->peakListWidget,&PeakListWidget::scanSelected,ui->analysisWidget,&AnalysisWidget::loadScan);
    connect(ui->analysisWidget,&AnalysisWidget::scanChanged,ui->peakListWidget,&PeakListWidget::selectScan);
    connect(ui->analysisWidget,&AnalysisWidget::peakAddRequested,ui->peakListWidget,&PeakListWidget::addUniqueLine);
	connect(ui->tabWidget,&QTabWidget::currentChanged,[=](int i){
        if(i == ui->tabWidget->indexOf(ui->logTab))
		{
			setLogIcon(QtFTM::LogNormal);
			if(d_logCount > 0)
			{
				d_logCount = 0;
                ui->tabWidget->setTabText(ui->tabWidget->indexOf(ui->logTab),QString("Log"));
			}
		}
	});
    connect(res1kHzAction,&QAction::triggered,[=](){ resolutionChanged(QtFTM::Res_1kHz); });
    connect(res2kHzAction,&QAction::triggered,[=](){ resolutionChanged(QtFTM::Res_2kHz); });
    connect(res5kHzAction,&QAction::triggered,[=](){ resolutionChanged(QtFTM::Res_5kHz); });
    connect(res10kHzAction,&QAction::triggered,[=](){ resolutionChanged(QtFTM::Res_10kHz); });
    connect(ui->saveLogButton,&QAbstractButton::clicked,this,&MainWindow::saveLogCallback);
    connect(ui->activateLogOnErrorBox,&QCheckBox::toggled,this,&MainWindow::logOnErrorCallback);

	//prepare status bar
	statusLabel = new QLabel();
	ui->statusBar->addWidget(statusLabel,4);
	ui->statusBar->addPermanentWidget(new QLabel(QString("Mirror Position: ")));

	mirrorProgress = new QProgressBar();
    mirrorProgress->setRange(-100000,100000);
	mirrorProgress->setTextVisible(false);
    ui->statusBar->addPermanentWidget(mirrorProgress,1);

	//log handler
	lh = new LogHandler(this);
	connect(lh,&LogHandler::sendLogMessage,ui->log,&QTextEdit::append);
	connect(lh,&LogHandler::sendLogMessage,[=](){
        if(ui->tabWidget->currentIndex() != ui->tabWidget->indexOf(ui->logTab))
		{
			d_logCount++;
            ui->tabWidget->setTabText(ui->tabWidget->indexOf(ui->logTab),QString("Log (%1)").arg(d_logCount));
		}
	});
	connect(lh,&LogHandler::sendStatusMessage,statusLabel,&QLabel::setText);
	connect(lh,&LogHandler::iconUpdate,this,&MainWindow::setLogIcon);

	acquisitionThread = new QThread(this);
	controlThread = new QThread(this);

    p_hwm = new HardwareManager();
    connect(this,&MainWindow::scopeResolutionChanged,p_hwm,&HardwareManager::scopeResolutionChanged);
    connect(p_hwm,&HardwareManager::ftmSynthUpdate,this,&MainWindow::ftmCavityUpdate);
    connect(p_hwm,&HardwareManager::probeFreqUpdate,this,&MainWindow::ftmProbeUpdate);
    connect(p_hwm,&HardwareManager::synthRangeChanged,this,&MainWindow::synthSettingsChanged);
    connect(p_hwm,&HardwareManager::drSynthFreqUpdate,this,&MainWindow::drSynthFreqUpdate);
    connect(p_hwm,&HardwareManager::drSynthPwrUpdate,this,&MainWindow::drSynthPwrUpdate);
    connect(p_hwm,&HardwareManager::attenUpdate,this,&MainWindow::attnUpdate);
    connect(p_hwm,&HardwareManager::taattenUpdate,this,&MainWindow::taattnUpdate);
    connect(p_hwm,&HardwareManager::attnTablePrepComplete,this,&MainWindow::attnTablePrepComplete);
    connect(p_hwm,&HardwareManager::logMessage,lh,&LogHandler::logMessage);
    connect(p_hwm,&HardwareManager::statusMessage,lh,&LogHandler::sendStatusMessage);
    connect(p_hwm,&HardwareManager::allHardwareConnected,this,&MainWindow::hardwareStatusChanged);
    connect(ui->ftmControlDoubleSpinBox,doubleVc,p_hwm,&HardwareManager::setFtmCavityFreqFromUI);
    connect(ui->drControlDoubleSpinBox,doubleVc,p_hwm,&HardwareManager::setDrSynthFreqFromUI);
    connect(ui->pwrControlDoubleSpinBox,doubleVc,p_hwm,&HardwareManager::setDrSynthPwrFromUI);
    connect(ui->attnControlSpinBox,intVc,p_hwm,&HardwareManager::setAttnFromUI);
    connect(p_hwm,&HardwareManager::attenUpdate,this,&MainWindow::setcvUpdate);
    connect(ui->dcControlSpinBox,intVc,p_hwm,&HardwareManager::setDcVoltageFromUI);
    connect(p_hwm,&HardwareManager::dcVoltageUpdate,this,&MainWindow::dcVoltageUpdate);
    connect(ui->magnetOnOffButton,&QAbstractButton::toggled,p_hwm,&HardwareManager::setMagnetFromUI);
    connect(p_hwm,&HardwareManager::magnetUpdate,this,&MainWindow::magnetUpdate);
    connect(p_hwm,&HardwareManager::flowUpdate,this,&MainWindow::updateFlow);
    connect(p_hwm,&HardwareManager::flowNameUpdate,this,&MainWindow::updateFlowName);
    connect(p_hwm,&HardwareManager::pressureUpdate,ui->pressureDoubleSpinBox,&QDoubleSpinBox::setValue);
    connect(p_hwm,&HardwareManager::flowSetpointUpdate,this,&MainWindow::updateFlowSetpoint);
    connect(p_hwm,&HardwareManager::pressureSetpointUpdate,this,&MainWindow::pressureSetpointUpdate);
    connect(p_hwm,&HardwareManager::pressureControlMode,this,&MainWindow::pressureControlMode);
    connect(this,&MainWindow::changeGasName,p_hwm,&HardwareManager::setGasName);
    connect(this,&MainWindow::setFlowSetpoint,p_hwm,&HardwareManager::setFlowSetpoint);
    connect(ui->pressureControlBox,doubleVc,p_hwm,&HardwareManager::setPressureSetpoint);

    gl = static_cast<QGridLayout*>(ui->gasControlGroup->layout());
    QGridLayout *gl2 = static_cast<QGridLayout*>(ui->flowStatusBox->layout());
    QWidget *lastFocusWidget = nullptr;
    for(int i=0; i<QTFTM_FLOW_NUMCHANNELS; i++)
    {
        FlowWidgets fw;
        fw.nameEdit = new QLineEdit(this);
        connect(fw.nameEdit,&QLineEdit::editingFinished,[=](){
            QMetaObject::invokeMethod(p_hwm,"setFlowChannelName",Q_ARG(int,i),Q_ARG(QString,fw.nameEdit->text()));
        });

        fw.controlBox = new QDoubleSpinBox(this);
        fw.controlBox->setSuffix(QString(" sccm"));
        fw.controlBox->setRange(0.0,10000.0);
        fw.controlBox->setSpecialValueText(QString("Off"));
        fw.controlBox->setKeyboardTracking(false);
        connect(fw.controlBox,doubleVc,[=](double val){
            QMetaObject::invokeMethod(p_hwm,"setFlowSetpoint",Q_ARG(int,i),Q_ARG(double,val));
        });
        lastFocusWidget = fw.controlBox;

        fw.nameLabel = new QLabel(QString("Ch%1").arg(i+1),this);
        fw.nameLabel->setMinimumWidth(QFontMetrics(QFont(QString("sans-serif"))).width(QString("MMMMMMMM")));
        fw.nameLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        fw.led = new Led(this);

        fw.displayBox = new QDoubleSpinBox(this);
        fw.displayBox->setRange(-9999.9,9999.9);
        fw.displayBox->setDecimals(2);
        fw.displayBox->setSuffix(QString(" sccm"));
        fw.displayBox->blockSignals(true);
        fw.displayBox->setReadOnly(true);
        fw.displayBox->setFocusPolicy(Qt::ClickFocus);
        fw.displayBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

        d_flowWidgets.append(fw);

        gl->addWidget(new QLabel(QString::number(i+1),this),1+i,0,1,1);
        gl->addWidget(fw.nameEdit,i+1,1,1,1);
        gl->addWidget(fw.controlBox,i+1,2,1,1);

        gl2->addWidget(fw.nameLabel,i+1,0,1,1,Qt::AlignRight);
        gl2->addWidget(fw.displayBox,i+1,1,1,1);
        gl2->addWidget(fw.led,i+1,2,1,1);
    }
    gl->addWidget(new QLabel(QString("Pressure"),this),2+QTFTM_FLOW_NUMCHANNELS,1,1,1,Qt::AlignRight);
    gl->addWidget(ui->pressureControlBox,2+QTFTM_FLOW_NUMCHANNELS,2,1,1);
    gl->addWidget(new QLabel(QString("Pressure Control Mode"),this),3+QTFTM_FLOW_NUMCHANNELS,1,1,1,Qt::AlignRight);
    gl->addWidget(ui->pressureControlButton,3+QTFTM_FLOW_NUMCHANNELS,2,1,1);
    gl->addItem(new QSpacerItem(10,10,QSizePolicy::Minimum,QSizePolicy::Expanding),4+QTFTM_FLOW_NUMCHANNELS,0,1,3);
    if(lastFocusWidget != nullptr)
        setTabOrder(lastFocusWidget,ui->pressureControlBox);

    setTabOrder(ui->pressureControlBox,ui->pressureControlButton);
    setTabOrder(ui->pressureControlButton,ui->pulseConfigWidget);

    connect(ui->pressureControlButton,&QAbstractButton::clicked,p_hwm,&HardwareManager::setPressureControlMode);
    connect(ui->pressureControlButton,&QAbstractButton::toggled,[=](bool b){
	    if(b)
		    ui->pressureControlButton->setText(QString("On"));
	    else
		    ui->pressureControlButton->setText(QString("Off"));
    });

    connect(p_hwm,&HardwareManager::pGenChannelSetting,ui->pulseConfigWidget,&PulseConfigWidget::newSetting);
    connect(p_hwm,&HardwareManager::pGenChannelSetting,this,&MainWindow::updatePulseLed);
    connect(p_hwm,&HardwareManager::pGenConfigUpdate,ui->pulseConfigWidget,&PulseConfigWidget::newConfig);
    connect(p_hwm,&HardwareManager::pGenConfigUpdate,this,&MainWindow::updatePulseLeds);
    connect(p_hwm,&HardwareManager::repRateUpdate,ui->pulseConfigWidget,&PulseConfigWidget::newRepRate);
    connect(p_hwm,&HardwareManager::hmProtectionDelayUpdate,ui->pulseConfigWidget,&PulseConfigWidget::newProtDelay);
    connect(p_hwm,&HardwareManager::hmScopeDelayUpdate,ui->pulseConfigWidget,&PulseConfigWidget::newScopeDelay);
    connect(ui->pulseConfigWidget,&PulseConfigWidget::changeSetting,p_hwm,&HardwareManager::setPulseSetting);
    connect(ui->pulseConfigWidget,&PulseConfigWidget::changeRepRate,p_hwm,&HardwareManager::setRepRate);
    connect(ui->pulseConfigWidget,&PulseConfigWidget::changeProtDelay,p_hwm,&HardwareManager::setProtectionDelayFromUI);
    connect(ui->pulseConfigWidget,&PulseConfigWidget::changeScopeDelay,p_hwm,&HardwareManager::setScopeDelayFromUI);
    connect(p_hwm,&HardwareManager::mirrorPosUpdate,this,&MainWindow::mirrorPosUpdate);
    connect(p_hwm,&HardwareManager::tuningComplete,this,&MainWindow::tuningComplete);
    connect(ui->actionTune_Cavity,&QAction::triggered,this,&MainWindow::tuneCavityCallback);
    connect(ui->actionCalibrate,&QAction::triggered,this,&MainWindow::calibrateCavityCallback);
    connect(p_hwm,&HardwareManager::canTuneUp,ui->actionTune_Up,&QAction::setEnabled);
    connect(p_hwm,&HardwareManager::canTuneDown,ui->actionTune_Down,&QAction::setEnabled);
    connect(p_hwm,&HardwareManager::modeChanged,this,&MainWindow::modeChanged);
    connect(p_hwm,&HardwareManager::tuningVoltageChanged,this,&MainWindow::tuningVoltageChanged);
    connect(ui->actionTune_Up,&QAction::triggered,this,&MainWindow::tuneUpCallback);
    connect(ui->actionTune_Down,&QAction::triggered,this,&MainWindow::tuneDownCallback);

    connect(controlThread,&QThread::started,p_hwm,&HardwareManager::initializeHardware);
    p_hwm->moveToThread(controlThread);

	sm = new ScanManager();

	connect(sm,&ScanManager::logMessage,lh,&LogHandler::logMessage);
	connect(sm,&ScanManager::statusMessage,lh,&LogHandler::sendStatusMessage);
	connect(sm,&ScanManager::peakUpFid,ui->peakUpPlot,&FtPlot::newFid);
	connect(sm,&ScanManager::scanFid,ui->acqFtPlot,&FtPlot::newFid);
	connect(sm,&ScanManager::initializationComplete,ui->scanSpinBox,&QAbstractSpinBox::stepUp);
    connect(sm,&ScanManager::scanShotAcquired,this,&MainWindow::updateScanProgressBar);
	connect(sm,&ScanManager::fatalSaveError,ui->scanSpinBox,&QAbstractSpinBox::stepDown);
	connect(ui->actionPause,&QAction::triggered,sm,&ScanManager::pause);
	connect(ui->actionResume,&QAction::triggered,sm,&ScanManager::resume);
	connect(ui->actionAbort,&QAction::triggered,sm,&ScanManager::abortScan);
	connect(ui->rollingAvgsSpinBox,intVc,sm,&ScanManager::setPeakUpAvgs);
	connect(ui->resetRollingAvgsButton,&QAbstractButton::clicked,sm,&ScanManager::resetPeakUpAvgs);
    connect(p_hwm,&HardwareManager::scopeWaveAcquired,sm,&ScanManager::fidReceived);
    connect(sm,&ScanManager::initializeHardwareForScan,p_hwm,&HardwareManager::prepareForScan);
    connect(p_hwm,&HardwareManager::scanInitialized,sm,&ScanManager::startScan);
    connect(p_hwm,&HardwareManager::probeFreqUpdate,sm,&ScanManager::setCurrentProbeFreq);
    connect(p_hwm,&HardwareManager::failure,sm,&ScanManager::failure);
    connect(p_hwm,&HardwareManager::retryScan,sm,&ScanManager::retryScan);


	sm->moveToThread(acquisitionThread);

	connect(ui->actionStart_Single,&QAction::triggered,this,&MainWindow::singleScanCallback);
	connect(ui->actionStart_Batch,&QAction::triggered,this,&MainWindow::batchScanCallback);

	batchThread = new QThread();

	acquisitionThread->start();
	controlThread->start();

	ui->peakUpMaxValueBox->blockSignals(true);
	ui->ftmDoubleSpinBox->blockSignals(true);
	ui->attenuationSpinBox->blockSignals(true);
    ui->cvSpinBox->blockSignals(true);
    ui->taSpinBox->blockSignals(true);
    ui->tvSpinBox->blockSignals(true);
	ui->drDoubleSpinBox->blockSignals(true);
	ui->powerDoubleSpinBox->blockSignals(true);
	ui->dcSpinBox->blockSignals(true);
	ui->scanSpinBox->blockSignals(true);

	connect(ui->peakUpPlot,&FtPlot::newFtMax,ui->peakUpMaxValueBox,&QDoubleSpinBox::setValue);

	ui->scanSpinBox->setValue(s.value(QString("scanNum"),0).toInt());

	ui->batchProgressBar->setValue(0);
	ui->shotsProgressBar->setValue(0);

    synthSettingsChanged();


	d_uiState = Idle;
	updateUiConfig();

}

MainWindow::~MainWindow()
{

	acquisitionThread->quit();
	acquisitionThread->wait();
	delete sm;

	controlThread->quit();
	controlThread->wait();
    delete p_hwm;

	delete ui;

}

void MainWindow::updateScanProgressBar()
{
	ui->shotsProgressBar->setValue(ui->shotsProgressBar->value()+1);
}

void MainWindow::updateBatchProgressBar()
{
    ui->batchProgressBar->setValue(ui->batchProgressBar->value()+1);
}

void MainWindow::updateUiConfig()
{
    if(d_hardwareConnected)
	{
	   ui->actionStart_Single->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->actionStart_Batch->setDisabled(d_uiState & (Acquiring|Tuning)  || d_uiState & Asleep);
	   ui->actionPause->setEnabled((d_uiState & Acquiring) && !(d_uiState & Paused) && !(d_uiState & Asleep));
	   ui->actionResume->setEnabled((d_uiState & Acquiring) && (d_uiState & Paused) && !(d_uiState & Asleep));
	   ui->actionAbort->setEnabled(d_uiState & Acquiring && !(d_uiState & Asleep));
	   ui->actionSleep_Mode->setDisabled(d_uiState & (Acquiring|Tuning));
       ui->actionDelayed_Sleep->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->actionCommunication->setDisabled(d_uiState & (Acquiring|Tuning));
	   ui->actionFT_Synth->setDisabled(d_uiState & (Acquiring|Tuning));
	   ui->actionDR_Synth->setDisabled(d_uiState & (Acquiring|Tuning));
	   ui->actionIO_Board->setDisabled(d_uiState & (Acquiring|Tuning));
        ui->actionView_Batch->setEnabled(true); //old batch scans can now be loaded at any time
	   ui->actionChange_Tuning_File->setDisabled(d_uiState & (Acquiring|Tuning));
	   ui->actionGenerate_Tuning_Table->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->synthControlGroup->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->environmentControlBox->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->gasControlGroup->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->pulseConfigWidget->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->menuResolution->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	   ui->menuMotor_Driver->setDisabled(d_uiState & (Acquiring|Tuning) || d_uiState & Asleep);
	}
	else
	{
		ui->actionStart_Single->setEnabled(false);
        ui->actionStart_Batch->setEnabled(false);
		ui->actionPause->setEnabled(false);
		ui->actionResume->setEnabled(false);
		ui->actionAbort->setEnabled(false);
		ui->actionSleep_Mode->setEnabled(false);
        ui->actionDelayed_Sleep->setEnabled(false);
		ui->actionCommunication->setEnabled(true);
		ui->actionFT_Synth->setEnabled(false);
        ui->actionDR_Synth->setEnabled(false);
        ui->actionIO_Board->setEnabled(true);
        ui->actionView_Batch->setEnabled(true);
        ui->actionChange_Tuning_File->setEnabled(true);
        ui->actionGenerate_Tuning_Table->setEnabled(false);
		ui->synthControlGroup->setEnabled(false);
		ui->environmentControlBox->setEnabled(false);
		ui->gasControlGroup->setEnabled(false);
		ui->pulseConfigWidget->setEnabled(false);
        ui->menuResolution->setEnabled(false);
        ui->menuMotor_Driver->setEnabled(false);
	}
}

void MainWindow::saveLogCallback()
{
    QString fileName = QFileDialog::getSaveFileName(this,QString("Save experiment log"),QDir::homePath().append(QString("/qtftm_%1.log").arg(QDateTime::currentDateTime().toString(Qt::ISODate))));
    if(!fileName.isEmpty())
    {
        QFile f(fileName);
        if(f.open(QIODevice::WriteOnly) && f.isWritable())
        {
            f.write(ui->log->document()->toPlainText().toLatin1());
            f.close();
            statusLabel->setText(QString("Log written to %1.").arg(fileName));
        }
        else
            QMessageBox::critical(this,QString("Saving failed!"),QString("Could not open file for writing.\n\nFile: %1").arg(fileName),QMessageBox::Ok);
    }
}

void MainWindow::logOnErrorCallback(bool ch)
{
    QSettings s(QSettings::SystemScope, QApplication::organizationName(),QApplication::applicationName());
    s.setValue(QString("activateLogOnError"),ch);
    s.sync();
}

void MainWindow::scanStarting(Scan s, bool isCal)
{
    ui->acqFtPlot->setCalVisible(isCal);
	ui->shotsProgressBar->setValue(0);
	ui->shotsProgressBar->setMaximum(s.targetShots());
}

void MainWindow::batchComplete(bool aborted)
{
    disconnect(sm,&ScanManager::scanShotAcquired,this,&MainWindow::updateBatchProgressBar);

	ui->actionPrint_Summary->setEnabled(true);
    d_uiState = Idle;
	updateUiConfig();
	if(aborted)
		statusLabel->setText(QString("Acquisition aborted"));
	else
		statusLabel->setText(QString("Acquisition complete"));

    QApplication::restoreOverrideCursor();
}

void MainWindow::pauseAcq()
{
	d_uiState |= Paused;
	updateUiConfig();
}

void MainWindow::resumeAcq()
{
	d_uiState ^= Paused;
    updateUiConfig();
}

void MainWindow::synthSettingsChanged()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("ftmSynth"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());

    ui->ftmControlDoubleSpinBox->blockSignals(true);
    ui->ftmControlDoubleSpinBox->setRange(s.value(QString("min"),5000.0).toDouble(),s.value(QString("max"),26490.0).toDouble());
    ui->ftmControlDoubleSpinBox->blockSignals(false);

    s.endGroup();
    s.endGroup();

    s.beginGroup(QString("drSynth"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());

    ui->drControlDoubleSpinBox->blockSignals(true);
    ui->pwrControlDoubleSpinBox->blockSignals(true);

    ui->drControlDoubleSpinBox->setRange(s.value(QString("min"),50.0).toDouble(),s.value(QString("max"),26500.0).toDouble());
    ui->pwrControlDoubleSpinBox->setRange(s.value(QString("minPower"),-70.0).toDouble(),s.value(QString("maxPower"),17.0).toDouble());

    ui->drControlDoubleSpinBox->blockSignals(false);
    ui->pwrControlDoubleSpinBox->blockSignals(false);

    s.endGroup();
    s.endGroup();

}

void MainWindow::ftmCavityUpdate(double f)
{
	ui->ftmControlDoubleSpinBox->blockSignals(true);
	ui->ftmControlDoubleSpinBox->setValue(f);
	ui->ftmControlDoubleSpinBox->blockSignals(false);
}

void MainWindow::ftmProbeUpdate(double f)
{
	ui->ftmDoubleSpinBox->setValue(f);
}

void MainWindow::drSynthFreqUpdate(double f)
{
	ui->drDoubleSpinBox->setValue(f);

	ui->drControlDoubleSpinBox->blockSignals(true);
	ui->drControlDoubleSpinBox->setValue(f);
	ui->drControlDoubleSpinBox->blockSignals(false);
}

void MainWindow::attnUpdate(int a)
{
	ui->attenuationSpinBox->setValue(a);

	ui->attnControlSpinBox->blockSignals(true);
	ui->attnControlSpinBox->setValue(a);
    ui->attnControlSpinBox->blockSignals(false);
}



void MainWindow::taattnUpdate(int a)
{
    ui->taSpinBox->setValue(a);
    ui->attenuationSpinBox->setValue(a);

// necessary?
    ui->attnControlSpinBox->blockSignals(true);
    ui->attnControlSpinBox->setValue(a);
    ui->attnControlSpinBox->blockSignals(false);
    //
}

void MainWindow::setcvUpdate(int a)
{

    double tvvalue,tavalue,cavalue,cvvalue;

   tvvalue=ui->tvSpinBox->value();
   tavalue=ui->taSpinBox->value();
   cavalue= (double) a;

   cvvalue= tvvalue*pow(10.0,(tavalue-cavalue)/10.0);

   ui->cvSpinBox->setValue((int) cvvalue);

}

void MainWindow::magnetUpdate(bool mag)
{
	ui->magnetOnOffButton->blockSignals(true);
	ui->magnetOnOffButton->setChecked(mag);
	if(mag)
		ui->magnetOnOffButton->setText(QString("On"));
	else
		ui->magnetOnOffButton->setText(QString("Off"));

	ui->magnetLed->setState(mag);
	ui->magnetOnOffButton->blockSignals(false);

}

void MainWindow::viewBatchCallback()
{

    LoadBatchDialog d(this);

    if(d.exec() == QDialog::Rejected)
        return;

    QPair<QtFTM::BatchType,int> result = d.selection();
    AbstractFitter *ftr = d.fitter();

    if(result.first == QtFTM::SingleScan || result.second < 1)
        return;

    //note: in the BatchViewWidget constructor, the Qt::WA_DeleteOnClose flag is set, so it will be deleted when the window is closed!
    BatchViewWidget *bvw = new BatchViewWidget(result.first,result.second,ftr);
    connect(this,&MainWindow::closing,bvw,&QWidget::close);
    connect(ui->analysisWidget,&AnalysisWidget::metaDataChanged,bvw,&BatchViewWidget::checkForMetaDataChanged);
    connect(bvw,&BatchViewWidget::metaDataChanged,ui->analysisWidget,&AnalysisWidget::checkForLoadScanMetaData);
    bvw->show();
    bvw->raise();
    bvw->process();

    delete ftr;

}

void MainWindow::mirrorPosUpdate(int pos)
{
    if(mirrorProgress->minimum() < 0 && mirrorProgress->maximum() > 0)
        mirrorProgress->setValue(pos);
}

void MainWindow::updateFlow(int ch, double val)
{
    if(ch < 0 || ch >= d_flowWidgets.size())
        return;

    d_flowWidgets.at(ch).displayBox->setValue(val);
}

void MainWindow::updateFlowName(int ch, QString name)
{
    if(ch < 0 || ch >= d_flowWidgets.size())
        return;

    if(name.isEmpty())
        d_flowWidgets.at(ch).nameLabel->setText(QString("Ch%1").arg(ch+1));
    else
    {
        d_flowWidgets.at(ch).nameLabel->setText(name.mid(0,9));
        d_flowWidgets.at(ch).nameEdit->blockSignals(true);
        d_flowWidgets.at(ch).nameEdit->setText(name);
        d_flowWidgets.at(ch).nameEdit->blockSignals(false);
    }
}

void MainWindow::updateFlowSetpoint(int ch, double val)
{
    if(ch < 0 || ch >= d_flowWidgets.size())
        return;

    d_flowWidgets.at(ch).controlBox->blockSignals(true);
    d_flowWidgets.at(ch).controlBox->setValue(val);
    d_flowWidgets.at(ch).controlBox->blockSignals(false);

    if(qFuzzyCompare(1.0,val+1.0))
        d_flowWidgets.at(ch).led->setState(false);
    else
        d_flowWidgets.at(ch).led->setState(true);
}

void MainWindow::pressureSetpointUpdate(double d)
{
    ui->pressureControlBox->blockSignals(true);
    ui->pressureControlBox->setValue(d);
    ui->pressureControlBox->blockSignals(false);
}

void MainWindow::pressureControlMode(bool en)
{
    ui->pressureControlButton->setChecked(en);
    ui->pressureLed->setState(en);
}

void MainWindow::singleScanCallback()
{
	if(batchThread->isRunning())
		return;

	//launch single scan dialog...
	SingleScanDialog d(this);
	d.setWindowTitle(QString("Single Scan"));

    AutoFitWidget *aw = new AutoFitWidget(guessBufferString(),ui->peakUpPlot->getDelay(),ui->peakUpPlot->getHpf(),ui->peakUpPlot->getExp(),ui->peakUpPlot->getPadFidBox()->isChecked(),ui->peakUpPlot->isUseWindow(),293.15,&d);
	d.insertAutoFitWidget(aw);

	SingleScanWidget *ssw = d.ssWidget();

	ssw->setFtmFreq(ui->ftmControlDoubleSpinBox->value());
	ssw->setAttn(ui->attnControlSpinBox->value());
	ssw->setDrFreq(ui->drControlDoubleSpinBox->value());
	ssw->setDrPower(ui->pwrControlDoubleSpinBox->value());
	ssw->setPulseConfig(ui->pulseConfigWidget->getConfig());
    ssw->setProtectionTime(ui->pulseConfigWidget->protDelay());
    ssw->setScopeTime(ui->pulseConfigWidget->scopeDelay());
    ssw->setMagnet(ui->magnetOnOffButton->isChecked());
    ssw->setDcVoltage(ui->dcControlSpinBox->value());
    ssw->enableSkipTune();

	int ret = d.exec();

	if(ret == QDialog::Rejected)
		return;

    Scan scan = ssw->toScan();
    AbstractFitter *af = aw->toFitter();

	BatchManager *bm = new BatchSingle(scan,af);

	startBatchManager(bm);

}

void MainWindow::batchScanCallback()
{
	if(batchThread->isRunning())
		return;

	SingleScanWidget *ssw = new SingleScanWidget();

	ssw->setFtmFreq(ui->ftmControlDoubleSpinBox->value());
	ssw->setAttn(ui->attnControlSpinBox->value());
	ssw->setDrFreq(ui->drControlDoubleSpinBox->value());
	ssw->setDrPower(ui->pwrControlDoubleSpinBox->value());
	ssw->setPulseConfig(ui->pulseConfigWidget->getConfig());
    ssw->setProtectionTime(ui->pulseConfigWidget->protDelay());
    ssw->setScopeTime(ui->pulseConfigWidget->scopeDelay());
    ssw->setMagnet(ui->magnetOnOffButton->isChecked());
    ssw->setDcVoltage(ui->dcControlSpinBox->value());

    AutoFitWidget *aw = new AutoFitWidget(guessBufferString(),ui->peakUpPlot->getDelay(),ui->peakUpPlot->getHpf(),ui->peakUpPlot->getExp(),ui->peakUpPlot->getPadFidBox()->isChecked());

	BatchWizard wiz(ssw,aw,this);

	connect(&wiz,&BatchWizard::setupDr,sm,&ScanManager::prepareScan);
    connect(sm,&ScanManager::dummyComplete,&wiz,&BatchWizard::drPrepComplete);
	connect(sm,&ScanManager::peakUpFid,&wiz,&BatchWizard::newFid);
	connect(&wiz,&BatchWizard::resetPeakUp,ui->resetRollingAvgsButton,&QAbstractButton::click);
	connect(&wiz,&BatchWizard::changeNumPeakupFids,ui->rollingAvgsSpinBox,&QSpinBox::setValue);

	int result = wiz.exec();

    if(result != QDialog::Accepted)
		return;

	BatchManager *bm = wiz.batchManager();

	if(bm == nullptr)
		return;

	startBatchManager(bm);

	ssw->deleteLater();
	aw->deleteLater();
}

void MainWindow::sleep(bool b)
{
	if(batchThread->isRunning()) // this should never happen, but we don't want to allow user to activate sleep mode during a scan!
	{
		ui->actionSleep_Mode->blockSignals(true);
		ui->actionSleep_Mode->setChecked(false);
		ui->actionSleep_Mode->setEnabled(false);
		ui->actionSleep_Mode->blockSignals(false);
		return;
	}

	if(b)
		d_uiState = Asleep;
	else
		d_uiState = Idle;

    QMetaObject::invokeMethod(p_hwm,"sleep",Q_ARG(bool,b));
	updateUiConfig();
	if(b)
		QMessageBox::information(this,QString("Sleep Mode Active"),QString("Sleep mode is active! Toggle it off before proceeding. Also, don't forget to re-enable pressure control."),QMessageBox::Ok);

}

void MainWindow::delayedSleep()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    int min_ = s.value(QString("lastSleepDelay"),60).toInt();
    bool ok = false;
    int minutes = QInputDialog::getInt(this,QString("Delayed Sleep"),QString("Sleep after X minutes:"),min_,0,214783647,60,&ok);
    if(!ok)
        return;
    if(minutes == 0)
        ui->actionSleep_Mode->trigger();
    else
    {
        s.setValue(QString("lastSleepDelay"),minutes);
        QDialog d(this);
        int secs = minutes*60;
        d.setWindowTitle(QString("Delayed Sleep"));
        QDateTime dt = QDateTime::currentDateTime().addSecs(secs);

        QPushButton *cancelButton = new QPushButton(QString("Cancel"));
        connect(cancelButton,&QPushButton::clicked,&d,&QDialog::reject);

        QLabel *text = new QLabel(QString("Program will enter sleep mode at %1.\nPress cancel or close this window to abort.").arg(dt.toString()));
        QVBoxLayout *vbl = new QVBoxLayout;
        vbl->addWidget(text);
        vbl->addWidget(cancelButton);

        d.setLayout(vbl);

        p_sleepTimer->start(secs*1000);
        connect(p_sleepTimer,&QTimer::timeout,&d,&QDialog::accept);
        if(d.exec() == QDialog::Rejected)
            p_sleepTimer->stop();
    }
}

void MainWindow::hardwareStatusChanged(bool success)
{
    //show an error if we go from connected to disconnected
    if((d_hardwareConnected && !success) || (!d_hardwareConnected && !success))
		statusLabel->setText(QString("A hardware error occurred. See log for details."));
    else if(!d_hardwareConnected && success)
    {
        statusLabel->setText(QString("Initialization complete."));
        setHardwareRanges();
    }

    d_hardwareConnected = success;
	updateUiConfig();
}

void MainWindow::launchCommunicationDialog()
{
	CommunicationDialog d(this);
    connect(&d,&CommunicationDialog::testConnection,p_hwm,&HardwareManager::testObjectConnection);
    connect(p_hwm,&HardwareManager::testComplete,&d,&CommunicationDialog::testComplete);

	d.exec();
}

void MainWindow::launchFtSettings()
{
	FtSynthSettingsWidget *w = new FtSynthSettingsWidget;

	//connect band change signal here
    connect(w,&SynthSettingsWidget::bandChanged,p_hwm,&HardwareManager::ftmSynthChangeBandFromUi);

	launchSettingsDialog(w);
    synthSettingsChanged();
}

void MainWindow::launchDrSettings()
{
	DrSynthSettingsWidget *d = new DrSynthSettingsWidget;

	//connect band change signal here
    connect(d,&SynthSettingsWidget::bandChanged,p_hwm,&HardwareManager::drSynthChangeBandFromUi);

    launchSettingsDialog(d);
    synthSettingsChanged();
}

void MainWindow::launchIOBoardSettings()
{
    IOBoardConfigDialog d(this);

    connect(&d,&IOBoardConfigDialog::testConnection,p_hwm,&HardwareManager::testObjectConnection);
    connect(p_hwm,&HardwareManager::testComplete,&d,&IOBoardConfigDialog::testComplete);

    d.exec();
}

void MainWindow::resolutionChanged(QtFTM::ScopeResolution res)
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.setValue(QString("scope/%1/resolution").arg(s.value(QString("scope/subKey"),QString("virtual")).toString()),(int)res);
    s.sync();

    emit scopeResolutionChanged();

}

void MainWindow::tuningComplete()
{
    if(d_uiState & Tuning)
	   d_uiState ^= Tuning;

    if(mirrorProgress->minimum() == 0)
    {
        mirrorProgress->setRange(-100000,100000);
        mirrorProgress->setValue(0);
    }
    updateUiConfig();
}

void MainWindow::tuneCavityCallback()
{
    ui->ftmControlDoubleSpinBox->interpretText();
    double freq = ui->ftmControlDoubleSpinBox->value();
    d_uiState |= Tuning;
    updateUiConfig();
    QMetaObject::invokeMethod(p_hwm,"tuneCavity",Q_ARG(double,freq));
}

void MainWindow::calibrateCavityCallback()
{

    //make a simple dialog
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    double minFreq = s.value(QString("ftmSynth/min"),5000.0).toInt();
    double maxFreq = s.value(QString("ftmSynth/max"),26450.0).toInt();

    if(minFreq > 10029.6 || maxFreq < 10030.4)
    {
        QMessageBox::critical(this,QString("Attenuation Table Error"),QString("The attenuation table cannot be automatically generated because the synthesizer frequency range (%1-%2 MHz) does not allow for calibration at 10030 MHz.").arg(minFreq,1,'f').arg(maxFreq,1,'f'));
        return;
    }

    int ret = QMessageBox::question(this,QString("Calibrate cavity?"),QString("This will peak the cavity on mode 46 at 10 GHz, and reset the encoder. If you meant to tune the cavity at the current frequency, select \"Tune Cavity\" instead.\n\nDo you wish to proceed with calibration?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    if(ret == QMessageBox::Yes)
    {
        mirrorProgress->setRange(0,0);
	   d_uiState |= Tuning;
        updateUiConfig();
        QMetaObject::invokeMethod(p_hwm,"calibrateCavity");
    }
}

void MainWindow::modeChanged(int mode)
{
    ui->actionTune_Up->setText(QString("Tune Up (mode %1)").arg(mode+1));
    ui->actionTune_Down->setText(QString("Tune Down (mode %1)").arg(mode-1));
}

void MainWindow::tuneUpCallback()
{
    if(d_uiState==Idle)
    {
	   d_uiState |= Tuning;
        updateUiConfig();
        QMetaObject::invokeMethod(p_hwm,"changeCavityMode",Q_ARG(double,ui->ftmControlDoubleSpinBox->value()),Q_ARG(bool,true));
    }
}

void MainWindow::tuneDownCallback()
{
    if(d_uiState==Idle)
    {
	   d_uiState |= Tuning;
        updateUiConfig();
        QMetaObject::invokeMethod(p_hwm,"changeCavityMode",Q_ARG(double,ui->ftmControlDoubleSpinBox->value()),Q_ARG(bool,false));
    }
}

void MainWindow::tuningVoltageChanged(int v)
{
    ui->tvSpinBox->setValue(v);
    ui->cvSpinBox->setValue(v);
}

void MainWindow::setHardwareRanges()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());


    ui->ftmControlDoubleSpinBox->blockSignals(true);
    s.beginGroup(QString("ftmSynth"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    ui->ftmControlDoubleSpinBox->setRange(s.value(QString("min"),5000.0).toDouble(),s.value(QString("max"),26490.0).toDouble());
    s.endGroup();
    s.endGroup();
    ui->ftmControlDoubleSpinBox->blockSignals(false);

    //attenuators
    ui->attnControlSpinBox->blockSignals(true);
    s.beginGroup(QString("attn"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    ui->attnControlSpinBox->setRange(s.value(QString("min"),0).toInt(),s.value(QString("max"),70).toInt());
    s.endGroup();
    s.endGroup();
    ui->attnControlSpinBox->blockSignals(false);


    ui->drControlDoubleSpinBox->blockSignals(true);
    ui->pwrControlDoubleSpinBox->blockSignals(true);
    s.beginGroup(QString("drSynth"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    ui->drControlDoubleSpinBox->setRange(s.value(QString("min"),50.0).toDouble(),s.value(QString("max"),26500.0).toDouble());
    ui->pwrControlDoubleSpinBox->setRange(s.value(QString("minPower"),-70.0).toDouble(),s.value(QString("maxPower"),17.0).toDouble());
    s.endGroup();
    s.endGroup();
    ui->drControlDoubleSpinBox->blockSignals(false);
    ui->pwrControlDoubleSpinBox->blockSignals(false);

    s.beginGroup(QString("hvps"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    ui->dcControlSpinBox->blockSignals(true);
    ui->dcControlSpinBox->setMinimum(s.value(QString("min"),0).toInt());
    ui->dcControlSpinBox->setMaximum(s.value(QString("max"),2000).toInt());
    ui->dcControlSpinBox->blockSignals(false);
    s.endGroup();
    s.endGroup();

}

void MainWindow::changeAttnFileCallback()
{
    if(batchThread->isRunning())
        return;

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/tuningTables"));
    if(!d.exists())
        d.mkpath(d.absolutePath());

    QString attenFile = QFileDialog::getOpenFileName(this,QString("Select Attenuation Data File"),d.absolutePath(),QString("Attenuation Files (*.atn);;Text Files (*.txt);;All Files (*.*)"));

    if(!attenFile.isEmpty())
    {
       connect(p_hwm,&HardwareManager::attnLoadSuccess,this,&MainWindow::attnFileSuccess,Qt::UniqueConnection);
        QMetaObject::invokeMethod(p_hwm,"changeAttnFile",Q_ARG(QString,attenFile));
    }

}

void MainWindow::attnFileSuccess(bool success)
{
    if(success)
        statusLabel->setText(QString("Attenuation file loaded successfully."));
    else
	   statusLabel->setText(QString("Attenuation file could not be loaded. See log for details."));

    disconnect(p_hwm,&HardwareManager::attnLoadSuccess,this,&MainWindow::attnFileSuccess);
}

void MainWindow::genAttnTableCallback()
{
    d_uiState = Tuning;
    updateUiConfig();

    //make a simple dialog
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    int minAttn = s.value(QString("attn/min"),0).toInt();
    int maxAttn = s.value(QString("attn/max"),70).toInt();
    double minFreq = s.value(QString("ftmSynth/min"),5000.0).toInt();
    double maxFreq = s.value(QString("ftmSynth/max"),26450.0).toInt();

    if(minFreq > 10029.6 || maxFreq < 10030.4)
    {
        QMessageBox::critical(this,QString("Attenuation Table Error"),QString("The attenuation table cannot be automatically generated because the synthesizer frequency range (%1-%2 MHz) does not allow for calibration at 10030 MHz.").arg(minFreq,1,'f').arg(maxFreq,1,'f'));
        return;
    }

    QDialog d;
    d.setWindowTitle(QString("Attenuation Table Setup"));

    QVBoxLayout *vl = new QVBoxLayout(&d);
    QLabel *lab = new QLabel(&d);
    lab->setWordWrap(true);
    lab->setText(QString("This procedure will attempt to automatically determine the attenuation to use for tuning over a specified frequency range.\n\nFirst, calibration at 10 GHz will be performed using the attenuation you specify, and the attenuation will be adjusted to give a 1 V cavity response. This process may take 2-3 minutes, during which time the abort button will not work.\n\nWhen this procedure is complete, you will be prompted to enter the range you wish the table to cover."));
    vl->addWidget(lab);

    QFormLayout *fl = new QFormLayout();

    QSpinBox *attn10GHzBox = new QSpinBox(&d);
    attn10GHzBox->setMinimum(minAttn);
    attn10GHzBox->setMaximum(maxAttn);
    attn10GHzBox->setValue(ui->attnControlSpinBox->value());
    attn10GHzBox->setSingleStep(1);
    attn10GHzBox->setToolTip(QString("The initial attenuation to be used for calibration. If you're unsure what to use, try 20 dB."));
    attn10GHzBox->setSuffix(QString(" dB"));
    fl->addRow(QString("10 GHz Attn Guess"),attn10GHzBox);

    vl->addLayout(fl,1);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb->button(QDialogButtonBox::Ok),&QAbstractButton::clicked,&d,&QDialog::accept);
    connect(bb->button(QDialogButtonBox::Cancel),&QAbstractButton::clicked,&d,&QDialog::reject);
    vl->addWidget(bb);

    int ret = d.exec();
    if(ret != QDialog::Accepted)
        return;

    QMetaObject::invokeMethod(p_hwm,"prepareForAttnTableGeneration",Q_ARG(int,attn10GHzBox->value()));
}

void MainWindow::attnTablePrepComplete(bool success)
{
    if(success)
        statusLabel->setText(QString("Preparation for generating attenuation table completed successfully"));
    else
    {
        statusLabel->setText(QString("A problem occurred while preparing to generate attenuation table. See log for details."));
	   d_uiState = Idle;
        updateUiConfig();
        return;
    }

    //make a simple dialog
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    double minFreq = s.value(QString("ftmSynth/min"),5000.0).toInt();
    double maxFreq = s.value(QString("ftmSynth/max"),26450.0).toInt();

    QDialog d;
    d.setWindowTitle(QString("Attenuation Table Setup"));

    QVBoxLayout *vl = new QVBoxLayout(&d);
    QLabel *lab = new QLabel(&d);
    lab->setWordWrap(true);
    lab->setText(QString("Enter the range and step size for the attenuation table. Measurements will be made from 10 GHz - Max, then 10 GHz - Min, correcting the attenuation along the way.\n\nThe attenuation table will be saved in /home/data/tuningTables according to the filename you specify (a suffix will be added if there is a name conflict), and it will also be saved as a type of batch acquisition.\n\nIf you are not satisfied with the calibration, choose \"Cancel\" to abort, and try again with a different initial attenuation."));
    vl->addWidget(lab);

    QFormLayout *fl = new QFormLayout();

    QDoubleSpinBox *minFreqBox = new QDoubleSpinBox(&d);
    minFreqBox->setMinimum(minFreq);
    minFreqBox->setMaximum(10029.6);
    minFreqBox->setValue(minFreq);
    minFreqBox->setSingleStep(500.0);
    minFreqBox->setDecimals(2);
    minFreqBox->setToolTip(QString("Minimum cavity frequency to cover in the attenuation table. Frequencies below this value will use 0 dB attenuation."));
    minFreqBox->setSuffix(QString(" MHz"));
    fl->addRow(QString("Min Frequency"),minFreqBox);

    QDoubleSpinBox *maxFreqBox = new QDoubleSpinBox(&d);
    maxFreqBox->setMinimum(10030.4);
    maxFreqBox->setMaximum(maxFreq);
    maxFreqBox->setValue(maxFreq);
    maxFreqBox->setSingleStep(500.0);
    maxFreqBox->setDecimals(2);
    maxFreqBox->setToolTip(QString("Maximum cavity frequency to cover in the attenuation table. Frequencies above this value will use 0 dB attenuation."));
    maxFreqBox->setSuffix(QString(" MHz"));
    fl->addRow(QString("Max Frequency"),maxFreqBox);

    QDoubleSpinBox *stepBox = new QDoubleSpinBox(&d);
    stepBox->setMinimum(0.5);
    stepBox->setMaximum(1000.0);
    stepBox->setValue(50.0);
    stepBox->setSingleStep(10.0);
    stepBox->setDecimals(2);
    stepBox->setToolTip(QString("Frequency step between measured frequencies."));
    stepBox->setSuffix(QString(" MHz"));
    fl->addRow(QString("Step Size"),stepBox);

    QLineEdit *nameBox = new QLineEdit(&d);
    nameBox->setText(QString("table"));
    nameBox->setToolTip(QString("Base file name of attenuation table output file (*.atn). If the name is already used, a number will be appended. If left blank, \"table\" will be used."));
    fl->addRow(QString("Output File Name"),nameBox);

    vl->addLayout(fl,1);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb->button(QDialogButtonBox::Ok),&QAbstractButton::clicked,&d,&QDialog::accept);
    connect(bb->button(QDialogButtonBox::Cancel),&QAbstractButton::clicked,&d,&QDialog::reject);
    vl->addWidget(bb);

    if(d.exec() != QDialog::Accepted)
    {
        statusLabel->setText(QString("Attenuation table generation canceled."));
	   d_uiState = Idle;
        updateUiConfig();
        return;
    }

    Scan scan;
    scan.setTargetShots(0);
    scan.setFtFreq(10030.0);
    scan.setAttenuation(ui->attenuationSpinBox->value());
    scan.setDrFreq(ui->drControlDoubleSpinBox->value());
    scan.setDrPower(ui->powerDoubleSpinBox->value());
    scan.setPulseConfiguration(ui->pulseConfigWidget->getConfig());
    scan.setProtectionDelayTime(ui->pulseConfigWidget->protDelay());
    scan.setScopeDelayTime(ui->pulseConfigWidget->scopeDelay());

    BatchAttenuation *bm = new BatchAttenuation(minFreqBox->value(),maxFreqBox->value(),stepBox->value(),scan.attenuation(),scan,nameBox->text());
    bm->setSleepWhenComplete(false);

    startBatchManager(bm);
}

void MainWindow::attnTableBatchComplete(bool aborted)
{
    if(aborted)
        statusLabel->setText(QString("Attenuation table generation aborted. See log for details."));
    else
        statusLabel->setText(QString("Attenuation table generation completed successfully. To use the new table, select Settings-Attenuator-Change Tuning File"));

    ui->shotsProgressBar->setRange(0,1);
    ui->shotsProgressBar->setValue(1);
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString attenFile = s.value(QString("attn/file"),QString("")).toString();
    QMetaObject::invokeMethod(p_hwm,"changeAttnFile",Q_ARG(QString,attenFile));
    d_uiState = Idle;
    ui->actionPrint_Summary->setEnabled(true);
    updateUiConfig();


}

void MainWindow::setLogIcon(QtFTM::LogMessageCode c)
{
    if(ui->tabWidget->currentIndex() != ui->tabWidget->indexOf(ui->logTab))
	{
		switch(c) {
		case QtFTM::LogWarning:
			if(d_logIcon != QtFTM::LogError)
			{
                ui->tabWidget->setTabIcon(ui->tabWidget->indexOf(ui->logTab),QIcon(QString(":/icons/warning.png")));
				d_logIcon = c;
			}
			break;
		case QtFTM::LogError:
            ui->tabWidget->setTabIcon(ui->tabWidget->indexOf(ui->logTab),QIcon(QString(":/icons/error.png")));
			d_logIcon = c;
            if(ui->activateLogOnErrorBox->isChecked())
                ui->tabWidget->setCurrentIndex(ui->tabWidget->indexOf(ui->logTab));
			break;
		default:
			d_logIcon = c;
            ui->tabWidget->setTabIcon(ui->tabWidget->indexOf(ui->logTab),QIcon());
			break;
		}
	}
	else
	{
		d_logIcon = QtFTM::LogNormal;
        ui->tabWidget->setTabIcon(ui->tabWidget->indexOf(ui->logTab),QIcon());
	}
}

void MainWindow::updatePulseLeds(const PulseGenConfig cc)
{
	for(int i=0; i<d_ledList.size() && i < cc.size(); i++)
	{
		d_ledList.at(i).first->setText(cc.at(i).channelName);
		d_ledList.at(i).second->setState(cc.at(i).enabled);
	}
}

void MainWindow::updatePulseLed(int index, QtFTM::PulseSetting s, QVariant val)
{
	if(index < 0 || index >= d_ledList.size())
	    return;

	switch(s) {
	case QtFTM::PulseName:
	    d_ledList.at(index).first->setText(val.toString());
	    break;
	case QtFTM::PulseEnabled:
	    d_ledList.at(index).second->setState(val.toBool());
	    break;
	default:
	    break;
	}

}

void MainWindow::dcVoltageUpdate(int v)
{
	ui->dcSpinBox->setValue(v);

	ui->dcControlSpinBox->blockSignals(true);
	ui->dcControlSpinBox->setValue(v);
	ui->dcControlSpinBox->blockSignals(false);
}

void MainWindow::startBatchManager(BatchManager *bm)
{    
    if(bm->type() != QtFTM::SingleScan)
    {
        if(ui->batchSplitter->sizes().at(1) == 0)
        {
            QList<int> sizes(ui->batchSplitter->sizes());
            sizes[0]-=250;
            sizes[1]+=250;
            ui->batchSplitter->setSizes(sizes);
        }
    }
	QByteArray state = ui->batchPlotSplitter->saveState();
	delete ui->batchPlot;

	if(bm->type() == QtFTM::SingleScan)
		ui->batchPlot = new QWidget();
	else
	{
		AbstractBatchPlot *plot = nullptr;
		if(bm->type() == QtFTM::Attenuation)
			plot = new BatchAttnPlot(bm->number());
		else if(bm->type() == QtFTM::Batch)
			plot = new BatchScanPlot(bm->number());
		else if(bm->type() == QtFTM::DrScan)
			plot = new DrPlot(bm->number());
		else if(bm->type() == QtFTM::Survey)
			plot = new SurveyPlot(bm->number());
        else if(bm->type() == QtFTM::DrCorrelation || bm->type() == QtFTM::Amdor)
            plot = new DrCorrPlot(bm->number(),bm->type());
        else if(bm->type() == QtFTM::Categorize)
            plot = new CategoryPlot(bm->number());

		connect(plot,&AbstractBatchPlot::requestScan,ui->analysisWidget,&AnalysisWidget::loadScan);
		connect(ui->analysisWidget,&AnalysisWidget::scanChanged,plot,&AbstractBatchPlot::setSelectedZone);
		connect(plot,&AbstractBatchPlot::colorChanged,ui->analysisWidget->plot(),&FtPlot::changeColor);
		connect(plot,&AbstractBatchPlot::colorChanged,ui->acqFtPlot,&FtPlot::changeColor);
		connect(ui->actionPrint_Summary,&QAction::triggered,plot,&AbstractBatchPlot::print);
		connect(bm,&BatchManager::plotData,plot,&AbstractBatchPlot::receiveData);
		connect(bm,&BatchManager::batchComplete,plot,&AbstractBatchPlot::enableReplotting);

		ui->batchPlot = plot;
		ui->batchPlotSplitter->insertWidget(0,plot);
		ui->batchPlotSplitter->restoreState(state);
	}

	ui->actionPrint_Summary->setEnabled(false);
    connect(bm,&BatchManager::processingComplete,ui->analysisWidget,&AnalysisWidget::newScan);
    connect(bm,&BatchManager::processingComplete,ui->peakListWidget,&PeakListWidget::addScan);
	connect(bm,&BatchManager::batchComplete,bm,&QObject::deleteLater);
	connect(bm,&QObject::destroyed,batchThread,&QThread::quit);
    connect(bm,&BatchManager::beginScan,this,&MainWindow::scanStarting);
    connect(bm,&BatchManager::beginScan,sm,&ScanManager::prepareScan);
	connect(bm,&BatchManager::logMessage,lh,&LogHandler::logMessage);

	if(bm->type() == QtFTM::Attenuation)
	{
		connect(bm,&BatchManager::batchComplete,this,&MainWindow::attnTableBatchComplete);
		connect(sm,&ScanManager::dummyComplete,bm,&BatchManager::scanComplete);
        connect(static_cast<BatchAttenuation*>(bm),&BatchAttenuation::elementComplete,this,&MainWindow::updateScanProgressBar);
        connect(static_cast<BatchAttenuation*>(bm),&BatchAttenuation::elementComplete,this,&MainWindow::updateBatchProgressBar);
		connect(ui->actionAbort,&QAction::triggered,static_cast<BatchAttenuation*>(bm),&BatchAttenuation::abort);

		ui->shotsProgressBar->setRange(0,0);
		ui->shotsProgressBar->setValue(0);
	}
	else
	{
        if(bm->type() == QtFTM::Categorize || bm->type() == QtFTM::Amdor)
            connect(bm,&BatchManager::advanced,this,&MainWindow::updateBatchProgressBar,Qt::UniqueConnection);
        else
            connect(sm,&ScanManager::scanShotAcquired,this,&MainWindow::updateBatchProgressBar,Qt::UniqueConnection);
		connect(bm,&BatchManager::batchComplete,this,&MainWindow::batchComplete);
		connect(sm,&ScanManager::scanComplete,bm,&BatchManager::scanComplete);
	}

    ui->batchProgressBar->setValue(0);
    ui->batchProgressBar->setRange(0,bm->totalShots());

    //Don't clear peak list widget!
//    ui->peakListWidget->clearAll();



	ui->analysisWidget->plot()->clearRanges();

	if(bm->type() == QtFTM::DrScan)
	{
		QList<QPair<double,double> > ranges = dynamic_cast<BatchDR*>(bm)->integrationRanges();
		ui->acqFtPlot->attachIntegrationRanges(ranges);
		ui->analysisWidget->plot()->attachIntegrationRanges(ranges);
		connect(batchThread,&QThread::finished,ui->acqFtPlot,&FtPlot::clearRanges);
	}

    //configure/hide AMDOR widget
    if(p_amdorWidget != nullptr)
    {
        p_amdorWidget->deleteLater();
        p_amdorWidget = nullptr;
        ui->tabWidget->removeTab(ui->tabWidget->count()-1);
    }
    if(bm->type() == QtFTM::Amdor)
    {
        AmdorBatch *ab = static_cast<AmdorBatch*>(bm);
        p_amdorWidget = new AmdorWidget(AmdorData(ab->allFrequencies(),ab->matchThreshold()),ab->number(),this);
        p_amdorWidget->enableEditing(false);
        connect(ab,&AmdorBatch::newRefScan,p_amdorWidget,&AmdorWidget::newRefScan);
        connect(ab,&AmdorBatch::newDrScan,p_amdorWidget,&AmdorWidget::newDrScan);

        ui->tabWidget->addTab(p_amdorWidget,QIcon(QString(":/icons/amdor.png")),QString("AMDOR"));
    }


    connect(batchThread,&QThread::started,bm,&BatchManager::beginBatch);
    if(bm->sleepWhenComplete())
	    connect(batchThread,&QThread::finished,ui->actionSleep_Mode,&QAction::trigger,Qt::UniqueConnection);
    else
	   disconnect(batchThread,&QThread::finished,ui->actionSleep_Mode,&QAction::trigger);

    bm->moveToThread(batchThread);
    batchThread->start();

    d_uiState = Acquiring;
    updateUiConfig();

}

void MainWindow::launchSettingsDialog(SettingsWidget *w)
{
	SettingsDialog d(w,this);

	d.exec();
}

QString MainWindow::guessBufferString()
{
	QString out;
	double max = 0.0;
    for(int i=0; i<d_flowWidgets.size(); i++)
    {
        if(d_flowWidgets.at(i).controlBox->value() > max)
            out = d_flowWidgets.at(i).nameEdit->text();
	}

	return out;

}


void MainWindow::drSynthPwrUpdate(double p)
{
	ui->powerDoubleSpinBox->setValue(p);

	ui->pwrControlDoubleSpinBox->blockSignals(true);
	ui->pwrControlDoubleSpinBox->setValue(p);
	ui->pwrControlDoubleSpinBox->blockSignals(false);
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
	if(batchThread->isRunning())
	{
		ev->ignore();
		return;
	}

	emit closing();
	ev->accept();
}
