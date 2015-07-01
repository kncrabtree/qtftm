#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scan.h"
#include "singlescandialog.h"
#include "singlescanwidget.h"
#include <QTimer>
#include <QSettings>
#include "batchsingle.h"
#include "batchsurvey.h"
#include "batchdr.h"
#include "batch.h"
#include "batchwizard.h"
#include <QGraphicsObject>
#include "communicationdialog.h"
#include "settingsdialog.h"
#include "ftsynthsettingswidget.h"
#include "drsynthsettingswidget.h"
#include "ioboardconfigdialog.h"
#include "pulsegeneratorsettingsdialog.h"
#include <QMessageBox>
#include <QFileDialog>
#include "loadbatchdialog.h"
#include "batchviewwidget.h"
#include "batchattenuation.h"
#include <QDialog>
#include <QDialogButtonBox>
#include "lorentziandopplerlmsfitter.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), hardwareConnected(false)
{
	//register custom types with meta object system
	qRegisterMetaType<QVector<QPointF> >("QVector<QPointF>");
	qRegisterMetaType<Fid>("Fid");
	qRegisterMetaType<LogHandler::MessageCode>("LogHandler::MessageCode");
	qRegisterMetaType<Scan>("Scan");
	qRegisterMetaType<PulseGenerator::PulseChannelConfiguration>("PulseGenerator::PulseChannelConfiguration");
	qRegisterMetaType<PulseGenerator::Setting>("PulseGenerator::Setting");
	qRegisterMetaType<QList<PulseGenerator::PulseChannelConfiguration> >("QList<PulseGenerator::PulseChannelConfiguration>");
	qRegisterMetaType<BatchManager::BatchPlotMetaData>("BatchManager::BatchPlotMetaData");
	qRegisterMetaType<QList<QVector<QPointF> > >("QList<QVector<QPointF> >");
    qRegisterMetaType<FlowController::FlowChannels>("FlowController::FlowChannels");
    qRegisterMetaType<FlowController::FlowIndex>("FlowController::FlowIndex");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<FitResult>("FitResult");

	//build UI and make trivial connections
	ui->setupUi(this);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	ui->rollingAvgsSpinBox->setValue(s.value(QString("peakUpAvgs"),20).toInt());

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

    Oscilloscope::Resolution r = (Oscilloscope::Resolution)s.value(QString("scope/resolution"),(int)Oscilloscope::Res_5kHz).toInt();

    switch(r)
    {
    case Oscilloscope::Res_1kHz:
        res1kHzAction->setChecked(true);
        break;
    case Oscilloscope::Res_2kHz:
        res2kHzAction->setChecked(true);
        break;
    case Oscilloscope::Res_5kHz:
        res5kHzAction->setChecked(true);
        break;
    case Oscilloscope::Res_10kHz:
        res10kHzAction->setChecked(true);
        break;
    }

    ui->menuResolution->addActions(resGroup->actions());

    //recall gas names
    ui->g1lineEdit->setText(s.value(QString("gas1Name"),QString("")).toString());
    ui->g2lineEdit->setText(s.value(QString("gas2Name"),QString("")).toString());
    ui->g3lineEdit->setText(s.value(QString("gas3Name"),QString("")).toString());
    ui->g4lineEdit->setText(s.value(QString("gas4Name"),QString("")).toString());

    //apply ranges to synth control boxes
    synthSettingsChanged();

    applyPulseNames();

    d_delayBoxes.append(ui->gasDelayBox);
    d_delayBoxes.append(ui->dcDelayBox);
    d_delayBoxes.append(ui->mwDelayBox);
    d_delayBoxes.append(ui->drDelayBox);
   d_delayBoxes.append(ui->protDelayBox);
    d_delayBoxes.append(ui->aux2DelayBox);
    d_delayBoxes.append(ui->aux3DelayBox);
    d_delayBoxes.append(ui->aux4DelayBox);

    d_widthBoxes.append(ui->gasWidthBox);
    d_widthBoxes.append(ui->dcWidthBox);
    d_widthBoxes.append(ui->mwWidthBox);
    d_widthBoxes.append(ui->drWidthBox);
   d_widthBoxes.append(ui->protWidthBox);
    d_widthBoxes.append(ui->aux2WidthBox);
    d_widthBoxes.append(ui->aux3WidthBox);
    d_widthBoxes.append(ui->aux4WidthBox);

    d_onOffButtons.append(nullptr);
    d_onOffButtons.append(ui->dcPulseOnButton);
    d_onOffButtons.append(nullptr);
    d_onOffButtons.append(ui->drPulseOnButton);
   d_onOffButtons.append(ui->aux1PulseOnButton);
    d_onOffButtons.append(ui->aux2PulseOnButton);
    d_onOffButtons.append(ui->aux3PulseOnButton);
    d_onOffButtons.append(ui->aux4PulseOnButton);

    d_leds.append(ui->gasLed);
    d_leds.append(ui->dcLed);
    d_leds.append(ui->mwLed);
    d_leds.append(ui->drLed);
   d_leds.append(ui->aux1Led);
    d_leds.append(ui->aux2Led);
    d_leds.append(ui->aux3Led);
    d_leds.append(ui->aux4Led);

    auto doubleVc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    auto intVc = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);

    connect(ui->g1setPointBox,doubleVc,[=](double d){ setFlowSetPoint(FlowController::Flow1,d); });
    connect(ui->g2setPointBox,doubleVc,[=](double d){ setFlowSetPoint(FlowController::Flow2,d); });
    connect(ui->g3setPointBox,doubleVc,[=](double d){ setFlowSetPoint(FlowController::Flow3,d); });
    connect(ui->g4setPointBox,doubleVc,[=](double d){ setFlowSetPoint(FlowController::Flow4,d); });
    connect(ui->pressureSetPointBox,doubleVc,[=](double d){ setFlowSetPoint(FlowController::Pressure,d); });
	connect(ui->actionPause,&QAction::triggered,this,&MainWindow::pauseAcq);
	connect(ui->actionResume,&QAction::triggered,this,&MainWindow::resumeAcq);
	connect(ui->actionPrint_Scan,&QAction::triggered,ui->analysisWidget,&AnalysisWidget::print);
	connect(ui->actionPrint_Summary,&QAction::triggered,ui->batchPlot,&BatchPlot::printCallback);
	connect(ui->actionSleep_Mode,&QAction::triggered,this,&MainWindow::sleep);
	connect(ui->actionCommunication,&QAction::triggered,this,&MainWindow::launchCommunicationDialog);
	connect(ui->actionFT_Synth,&QAction::triggered,this,&MainWindow::launchFtSettings);
	connect(ui->actionDR_Synth,&QAction::triggered,this,&MainWindow::launchDrSettings);
    connect(ui->actionIO_Board,&QAction::triggered,this,&MainWindow::launchIOBoardSettings);
    connect(ui->actionPulse_Generator,&QAction::triggered,this,&MainWindow::launchPulseGeneratorSettings);
    connect(ui->actionView_Batch,&QAction::triggered,this,&MainWindow::viewBatchCallback);
    connect(ui->actionChange_Tuning_File,&QAction::triggered,this,&MainWindow::changeAttnFileCallback);
    connect(ui->actionGenerate_Tuning_Table,&QAction::triggered,this,&MainWindow::genAttnTableCallback);
	connect(ui->analysisWidget,&AnalysisWidget::canPrint,ui->actionPrint_Scan,&QAction::setEnabled);
    connect(ui->peakListWidget,&PeakListWidget::scanSelected,ui->analysisWidget,&AnalysisWidget::loadScan);
    connect(ui->analysisWidget,&AnalysisWidget::scanChanged,ui->peakListWidget,&PeakListWidget::selectScan);
    connect(ui->analysisWidget,&AnalysisWidget::peakAddRequested,ui->peakListWidget,&PeakListWidget::addUniqueLine);
	connect(ui->batchPlot,&BatchPlot::requestScan,ui->analysisWidget,&AnalysisWidget::loadScan);
	connect(ui->analysisWidget,&AnalysisWidget::scanChanged,ui->batchPlot,&BatchPlot::showZone);
	connect(ui->batchPlot,&BatchPlot::colorChanged,ui->analysisWidget->plot(),&FtPlot::changeColor);
	connect(ui->batchPlot,&BatchPlot::colorChanged,ui->acqFtPlot,&FtPlot::changeColor);
    connect(res1kHzAction,&QAction::triggered,[=](){ resolutionChanged(Oscilloscope::Res_1kHz); });
    connect(res2kHzAction,&QAction::triggered,[=](){ resolutionChanged(Oscilloscope::Res_2kHz); });
    connect(res5kHzAction,&QAction::triggered,[=](){ resolutionChanged(Oscilloscope::Res_5kHz); });
    connect(res10kHzAction,&QAction::triggered,[=](){ resolutionChanged(Oscilloscope::Res_10kHz); });
    connect(ui->saveLogButton,&QAbstractButton::clicked,this,&MainWindow::saveLogCallback);
    connect(ui->g1lineEdit,&QLineEdit::editingFinished,this,&MainWindow::gasNamesChanged);
    connect(ui->g2lineEdit,&QLineEdit::editingFinished,this,&MainWindow::gasNamesChanged);
    connect(ui->g3lineEdit,&QLineEdit::editingFinished,this,&MainWindow::gasNamesChanged);
    connect(ui->g4lineEdit,&QLineEdit::editingFinished,this,&MainWindow::gasNamesChanged);

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
	connect(lh,&LogHandler::sendStatusMessage,statusLabel,&QLabel::setText);

	acquisitionThread = new QThread(this);
	controlThread = new QThread(this);

	hwm = new HardwareManager();
	connect(hwm,&HardwareManager::ftmSynthUpdate,this,&MainWindow::ftmCavityUpdate);
	connect(hwm,&HardwareManager::probeFreqUpdate,this,&MainWindow::ftmProbeUpdate);
    connect(hwm,&HardwareManager::synthRangeChanged,this,&MainWindow::synthSettingsChanged);
#ifndef CONFIG_NODRSYNTH
	connect(hwm,&HardwareManager::drSynthFreqUpdate,this,&MainWindow::drSynthFreqUpdate);
	connect(hwm,&HardwareManager::drSynthPwrUpdate,this,&MainWindow::drSynthPwrUpdate);
#endif
    connect(hwm,&HardwareManager::attenUpdate,this,&MainWindow::attnUpdate);
    connect(hwm,&HardwareManager::taattenUpdate,this,&MainWindow::taattnUpdate);
    connect(hwm,&HardwareManager::attnTablePrepComplete,this,&MainWindow::attnTablePrepComplete);
    connect(hwm,&HardwareManager::logMessage,lh,&LogHandler::logMessage);
	connect(hwm,&HardwareManager::statusMessage,lh,&LogHandler::sendStatusMessage);
	connect(hwm,&HardwareManager::allHardwareConnected,this,&MainWindow::hardwareStatusChanged);
	connect(ui->ftmControlDoubleSpinBox,doubleVc,hwm,&HardwareManager::setFtmCavityFreqFromUI);
#ifndef CONFIG_NODRSYNTH
	connect(ui->drControlDoubleSpinBox,doubleVc,hwm,&HardwareManager::setDrSynthFreqFromUI);
	connect(ui->pwrControlDoubleSpinBox,doubleVc,hwm,&HardwareManager::setDrSynthPwrFromUI);
#endif
    connect(ui->protectionSpinBox,intVc,hwm,&HardwareManager::setProtectionDelayFromUI);
    connect(ui->scopeDelaySpinBox,intVc,hwm,&HardwareManager::setScopeDelayFromUI);
    connect(hwm,&HardwareManager::hmScopeDelayUpdate,this,&MainWindow::updateScopeDelayOnUI);
    connect(hwm,&HardwareManager::hmProtectionDelayUpdate,this,&MainWindow::updateProtectionDelayOnUI);
	connect(ui->attnControlSpinBox,intVc,hwm,&HardwareManager::setAttnFromUI);
    connect(hwm,&HardwareManager::attenUpdate,this,&MainWindow::setcvUpdate);
    connect(ui->magnetOnOffButton,&QAbstractButton::clicked,hwm,&HardwareManager::setMagnetFromUI);
    connect(hwm,&HardwareManager::magnetUpdate,this,&MainWindow::magnetUpdate);
    connect(hwm,&HardwareManager::flowUpdated,this,&MainWindow::flowControllerUpdate);
    connect(hwm,&HardwareManager::flowSetPointUpdated,this,&MainWindow::flowSetPointUpdated);
    connect(hwm,&HardwareManager::pressureControlMode,this,&MainWindow::pressureControlMode);
    connect(ui->pressureControlButton,&QAbstractButton::clicked,hwm,&HardwareManager::setPressureControlMode);
    connect(ui->pressureControlButton,&QAbstractButton::clicked,[=](bool b){
	    if(b)
		    ui->pressureControlButton->setText(QString("On"));
	    else
		    ui->pressureControlButton->setText(QString("Off"));
    });

    for(int i=0;i<8;i++)
    {
	    connect(d_delayBoxes.at(i),doubleVc,[=](double d){ pGenSet(i,PulseGenerator::Delay,d); });
	    connect(d_widthBoxes.at(i),doubleVc,[=](double d){ pGenSet(i,PulseGenerator::Width,d); });
	    if(d_onOffButtons.at(i))
	    {
		    connect(d_onOffButtons.at(i),&QPushButton::toggled,[=](bool b){ pGenSet(i,PulseGenerator::Enabled,b); });
		    connect(d_onOffButtons.at(i),&QPushButton::toggled,[=](bool on){
			    if(on)
				    d_onOffButtons[i]->setText(QString("On"));
			    else
				    d_onOffButtons[i]->setText(QString("Off"));
		    });
	    }
    }
	connect(hwm,&HardwareManager::pGenChannelSetting,this,&MainWindow::pGenSettingUpdate);
	connect(hwm,&HardwareManager::pGenChannelAll,this,&MainWindow::pGenChannelUpdate);
	connect(hwm,&HardwareManager::pGenAll,this,&MainWindow::pGenAllUpdate);
	connect(hwm,&HardwareManager::pGenAll,ui->pulsePlot,&PulsePlot::pConfigAll);
	connect(hwm,&HardwareManager::pGenChannelAll,ui->pulsePlot,&PulsePlot::pConfigSingle);
	connect(hwm,&HardwareManager::pGenChannelSetting,ui->pulsePlot,&PulsePlot::pConfigSetting);
    connect(hwm,&HardwareManager::mirrorPosUpdate,this,&MainWindow::mirrorPosUpdate);
    connect(hwm,&HardwareManager::tuningComplete,this,&MainWindow::tuningComplete);
    connect(ui->actionTune_Cavity,&QAction::triggered,this,&MainWindow::tuneCavityCallback);
    connect(ui->actionCalibrate,&QAction::triggered,this,&MainWindow::calibrateCavityCallback);
    connect(hwm,&HardwareManager::canTuneUp,ui->actionTune_Up,&QAction::setEnabled);
    connect(hwm,&HardwareManager::canTuneDown,ui->actionTune_Down,&QAction::setEnabled);
    connect(hwm,&HardwareManager::modeChanged,this,&MainWindow::modeChanged);
    connect(hwm,&HardwareManager::tuningVoltageChanged,this,&MainWindow::tuningVoltageChanged);
    connect(ui->actionTune_Up,&QAction::triggered,this,&MainWindow::tuneUpCallback);
    connect(ui->actionTune_Down,&QAction::triggered,this,&MainWindow::tuneDownCallback);

	connect(controlThread,&QThread::started,hwm,&HardwareManager::initializeHardware);
	hwm->moveToThread(controlThread);

	sm = new ScanManager();

	connect(sm,&ScanManager::logMessage,lh,&LogHandler::logMessage);
	connect(sm,&ScanManager::statusMessage,lh,&LogHandler::sendStatusMessage);
	connect(sm,&ScanManager::peakUpFid,ui->peakUpPlot,&FtPlot::newFid);
	connect(sm,&ScanManager::scanFid,ui->acqFtPlot,&FtPlot::newFid);
	connect(sm,&ScanManager::initializationComplete,ui->scanSpinBox,&QAbstractSpinBox::stepUp);
	connect(sm,&ScanManager::scanShotAcquired,this,&MainWindow::updateProgressBars);
	connect(sm,&ScanManager::fatalSaveError,this,&MainWindow::fatalSaveError);
	connect(sm,&ScanManager::fatalSaveError,ui->scanSpinBox,&QAbstractSpinBox::stepDown);
	connect(ui->actionPause,&QAction::triggered,sm,&ScanManager::pause);
	connect(ui->actionResume,&QAction::triggered,sm,&ScanManager::resume);
	connect(ui->actionAbort,&QAction::triggered,sm,&ScanManager::abortScan);
	connect(ui->rollingAvgsSpinBox,intVc,sm,&ScanManager::setPeakUpAvgs);
	connect(ui->resetRollingAvgsButton,&QAbstractButton::clicked,sm,&ScanManager::resetPeakUpAvgs);
	connect(hwm,&HardwareManager::scopeWaveAcquired,sm,&ScanManager::fidReceived);
	connect(sm,&ScanManager::initializeHardwareForScan,hwm,&HardwareManager::prepareForScan);
	connect(hwm,&HardwareManager::scanInitialized,sm,&ScanManager::startScan);
	connect(hwm,&HardwareManager::probeFreqUpdate,sm,&ScanManager::setCurrentProbeFreq);
    connect(hwm,&HardwareManager::failure,sm,&ScanManager::failure);
    connect(hwm,&HardwareManager::retryScan,sm,&ScanManager::retryScan);


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
	ui->g1DoubleSpinBox->blockSignals(true);
	ui->g2DoubleSpinBox->blockSignals(true);
	ui->g3DoubleSpinBox->blockSignals(true);
	ui->g4DoubleSpinBox->blockSignals(true);
	ui->pDoubleSpinBox->blockSignals(true);
	ui->dcSpinBox->blockSignals(true);
	ui->scanSpinBox->blockSignals(true);

	connect(ui->peakUpPlot,&FtPlot::newFtMax,ui->peakUpMaxValueBox,&QDoubleSpinBox::setValue);

	ui->scanSpinBox->setValue(s.value(QString("scanNum"),0).toInt());

	ui->batchProgressBar->setValue(0);
	ui->shotsProgressBar->setValue(0);

#ifdef CONFIG_NODRSYNTH
    ui->drControlDoubleSpinBox->setMinimum(-10.0);
    ui->drControlDoubleSpinBox->setValue(-1.0);
    ui->drDoubleSpinBox->setMinimum(-10.0);
    ui->drDoubleSpinBox->setValue(-1.0);
    ui->powerDoubleSpinBox->setValue(0.0);
    ui->pwrControlDoubleSpinBox->setValue(0.0);
    ui->drControlDoubleSpinBox->blockSignals(true);
    ui->pwrControlDoubleSpinBox->blockSignals(true);
#endif

    synthSettingsChanged();
	uiState = Idle;
	updateUiConfig();
    noHardwareMode = false;
}

MainWindow::~MainWindow()
{

	acquisitionThread->quit();
	acquisitionThread->wait();
	delete sm;

	controlThread->quit();
	controlThread->wait();
	delete hwm;

	delete ui;

}

void MainWindow::updateProgressBars()
{
	ui->shotsProgressBar->setValue(ui->shotsProgressBar->value()+1);
	ui->batchProgressBar->setValue(ui->batchProgressBar->value()+1);
}

void MainWindow::updateUiConfig()
{
    if(hardwareConnected || noHardwareMode)
	{
        ui->actionStart_Single->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
        ui->actionStart_Batch->setDisabled(uiState & (Acquiring|Tuning)  || uiState & Asleep);
        ui->actionPause->setEnabled((uiState & Acquiring) && !(uiState & Paused) && !(uiState & Asleep));
        ui->actionResume->setEnabled((uiState & Acquiring) && (uiState & Paused) && !(uiState & Asleep));
        ui->actionAbort->setEnabled(uiState & Acquiring && !(uiState & Asleep));
        ui->actionSleep_Mode->setDisabled(uiState & (Acquiring|Tuning));
        ui->actionCommunication->setDisabled(uiState & (Acquiring|Tuning));
        ui->actionFT_Synth->setDisabled(uiState & (Acquiring|Tuning));
        ui->actionDR_Synth->setDisabled(uiState & (Acquiring|Tuning));
        ui->actionIO_Board->setDisabled(uiState & (Acquiring|Tuning));
        ui->actionPulse_Generator->setDisabled(uiState & (Acquiring|Tuning));
        ui->actionView_Batch->setEnabled(true); //old batch scans can now be loaded at any time
        ui->actionChange_Tuning_File->setDisabled(uiState & (Acquiring|Tuning));
        ui->actionGenerate_Tuning_Table->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
        ui->synthControlGroup->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
        ui->environmentControlBox->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
        ui->gasControlGroup->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
        ui->pulseGroup->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
        ui->menuResolution->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
        ui->menuMotor_Driver->setDisabled(uiState & (Acquiring|Tuning) || uiState & Asleep);
	}
	else
	{
		ui->actionStart_Single->setEnabled(false);
        ui->actionStart_Batch->setEnabled(false);
		ui->actionPause->setEnabled(false);
		ui->actionResume->setEnabled(false);
		ui->actionAbort->setEnabled(false);
		ui->actionSleep_Mode->setEnabled(false);
		ui->actionCommunication->setEnabled(true);
		ui->actionFT_Synth->setEnabled(false);
        ui->actionDR_Synth->setEnabled(false);
        ui->actionIO_Board->setEnabled(true);
        ui->actionPulse_Generator->setEnabled(false);
        ui->actionView_Batch->setEnabled(true);
        ui->actionChange_Tuning_File->setEnabled(true);
        ui->actionGenerate_Tuning_Table->setEnabled(false);
		ui->synthControlGroup->setEnabled(false);
		ui->environmentControlBox->setEnabled(false);
		ui->gasControlGroup->setEnabled(false);
		ui->pulseGroup->setEnabled(false);
        ui->menuResolution->setEnabled(false);
        ui->menuMotor_Driver->setEnabled(false);
	}

#ifdef CONFIG_NODRSYNTH
    ui->actionDR_Synth->setEnabled(false);
    ui->drControlDoubleSpinBox->setEnabled(false);
    ui->pwrControlDoubleSpinBox->setEnabled(false);
#endif
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

void MainWindow::scanStarting(Scan s, bool isCal)
{
    ui->acqFtPlot->setCalVisible(isCal);
	ui->shotsProgressBar->setValue(0);
	ui->shotsProgressBar->setMaximum(s.targetShots());
}

void MainWindow::batchComplete(bool aborted)
{
	ui->actionPrint_Summary->setEnabled(true);
    uiState = Idle;
	updateUiConfig();
	if(aborted)
		statusLabel->setText(QString("Acquisition aborted"));
	else
		statusLabel->setText(QString("Acquisition complete"));

    QApplication::restoreOverrideCursor();
}

void MainWindow::pauseAcq()
{
	uiState |= Paused;
	updateUiConfig();
}

void MainWindow::resumeAcq()
{
	uiState ^= Paused;
    updateUiConfig();
}

void MainWindow::synthSettingsChanged()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

    ui->ftmControlDoubleSpinBox->blockSignals(true);
    ui->ftmControlDoubleSpinBox->setRange(s.value(QString("ftmSynth/min"),5000.0).toDouble(),s.value(QString("ftmSynth/max"),26490.0).toDouble());
    ui->ftmControlDoubleSpinBox->blockSignals(false);

#ifndef CONFIG_NODRSYNTH
    ui->drControlDoubleSpinBox->blockSignals(true);
    ui->pwrControlDoubleSpinBox->blockSignals(true);

    ui->drControlDoubleSpinBox->setRange(s.value(QString("drSynth/min"),50.0).toDouble(),s.value(QString("drSynth/max"),26500.0).toDouble());
    ui->pwrControlDoubleSpinBox->setRange(s.value(QString("drSynth/minPower"),-70.0).toDouble(),s.value(QString("drSynth/maxPower"),17.0).toDouble());

    ui->drControlDoubleSpinBox->blockSignals(false);
    ui->pwrControlDoubleSpinBox->blockSignals(false);
#endif

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

void MainWindow::updateScopeDelayOnUI(int a)
{
    ui->scopeDelaySpinBox->blockSignals(true);
    ui->scopeDelaySpinBox->setValue(a);
    ui->scopeDelaySpinBox->blockSignals(false);
}

void MainWindow::updateProtectionDelayOnUI(int a)
{
    ui->protectionSpinBox->blockSignals(true);
    ui->protectionSpinBox->setValue(a);
    ui->protectionSpinBox->blockSignals(false);
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

    QPair<BatchManager::BatchType,int> result = d.selection();

    if(result.first == BatchManager::SingleScan || result.second < 1)
        return;

    //note: in the BatchViewWidget constructor, the Qt::WA_DeleteOnClose flag is set, so it will be deleted when the window is closed!
    BatchViewWidget *bvw = new BatchViewWidget(result.first,result.second,d.delay(),d.hpf(),d.exp(),d.removeDC(),d.padFid());
    connect(this,&MainWindow::closing,bvw,&QWidget::close);
    connect(ui->analysisWidget,&AnalysisWidget::metaDataChanged,bvw,&BatchViewWidget::checkForMetaDataChanged);
    connect(bvw,&BatchViewWidget::metaDataChanged,ui->analysisWidget,&AnalysisWidget::checkForLoadScanMetaData);
    bvw->show();
    bvw->raise();
    bvw->process();

}

void MainWindow::mirrorPosUpdate(int pos)
{
    if(mirrorProgress->minimum() < 0 && mirrorProgress->maximum() > 0)
        mirrorProgress->setValue(pos);
}

void MainWindow::flowControllerUpdate(FlowController::FlowIndex i, double d)
{
    switch(i)
    {
    case FlowController::Pressure:
        ui->pDoubleSpinBox->setValue(d);
        break;
    case FlowController::Flow1:
        ui->g1DoubleSpinBox->setValue(d);
        break;
    case FlowController::Flow2:
        ui->g2DoubleSpinBox->setValue(d);
        break;
    case FlowController::Flow3:
        ui->g3DoubleSpinBox->setValue(d);
        break;
    case FlowController::Flow4:
        ui->g4DoubleSpinBox->setValue(d);
        break;
    }
}

void MainWindow::flowSetPointUpdated(FlowController::FlowIndex i, double d)
{
    switch(i)
    {
    case FlowController::Pressure:
        ui->pressureSetPointBox->blockSignals(true);
        ui->pressureSetPointBox->setValue(d);
        ui->pressureSetPointBox->blockSignals(false);
        break;
    case FlowController::Flow1:
        ui->g1setPointBox->blockSignals(true);
        ui->g1setPointBox->setValue(d);
        ui->g1setPointBox->blockSignals(false);
        break;
    case FlowController::Flow2:
        ui->g2setPointBox->blockSignals(true);
        ui->g2setPointBox->setValue(d);
        ui->g2setPointBox->blockSignals(false);
        break;
    case FlowController::Flow3:
        ui->g3setPointBox->blockSignals(true);
        ui->g3setPointBox->setValue(d);
        ui->g3setPointBox->blockSignals(false);
        break;
    case FlowController::Flow4:
        ui->g4setPointBox->blockSignals(true);
        ui->g4setPointBox->setValue(d);
        ui->g4setPointBox->blockSignals(false);
        break;
    }
}

void MainWindow::setFlowSetPoint(FlowController::FlowIndex i, double d)
{
	QMetaObject::invokeMethod(hwm,"setFlowSetPoint",Q_ARG(FlowController::FlowIndex,i),Q_ARG(double,d));
}

void MainWindow::pressureControlMode(bool on)
{
    if(on)
    {
        if(!ui->pressureControlButton->isChecked())
        {
            ui->pressureControlButton->blockSignals(true);
            ui->pressureControlButton->setChecked(true);
            ui->pressureControlButton->setText(QString("On"));
            ui->pressureControlButton->blockSignals(false);
        }
    }
    else
    {
        if(ui->pressureControlButton->isChecked())
        {
            ui->pressureControlButton->blockSignals(true);
            ui->pressureControlButton->setChecked(false);
            ui->pressureControlButton->setText(QString("Off"));
            ui->pressureControlButton->blockSignals(false);
        }
    }
}

void MainWindow::fatalSaveError()
{
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
}

void MainWindow::pGenSettingUpdate(const int ch, const PulseGenerator::Setting s, const QVariant val)
{
	if(ch-1 >= 8)
		return;

	switch(s)
	{
	case PulseGenerator::Delay:
		d_delayBoxes[ch-1]->blockSignals(true);
		d_delayBoxes[ch-1]->setValue(val.toDouble());
		d_delayBoxes[ch-1]->blockSignals(false);
		break;
	case PulseGenerator::Width:
		d_widthBoxes[ch-1]->blockSignals(true);
		d_widthBoxes[ch-1]->setValue(val.toDouble());
		d_widthBoxes[ch-1]->blockSignals(false);
		break;
	case PulseGenerator::Enabled:
		if(val.toBool())
			d_leds[ch-1]->setState(true);
		else
			d_leds[ch-1]->setState(false);

		if(d_onOffButtons.at(ch-1))
		{
			d_onOffButtons[ch-1]->blockSignals(true);
			d_onOffButtons[ch-1]->setChecked(val.toBool());
			if(val.toBool())
				d_onOffButtons[ch-1]->setText(QString("On"));
			else
				d_onOffButtons[ch-1]->setText(QString("Off"));
			d_onOffButtons[ch-1]->blockSignals(false);
		}
		break;
	default:
		break;
	}
}

void MainWindow::pGenChannelUpdate(const PulseGenerator::PulseChannelConfiguration p)
{
	pGenSettingUpdate(p.channel,PulseGenerator::Delay,QVariant(p.delay));
	pGenSettingUpdate(p.channel,PulseGenerator::Width,QVariant(p.width));
	pGenSettingUpdate(p.channel,PulseGenerator::Enabled,QVariant(p.enabled));
}

void MainWindow::pGenAllUpdate(const QList<PulseGenerator::PulseChannelConfiguration> l)
{
	for(int i=0;i<l.size();i++)
		pGenChannelUpdate(l.at(i));
}

void MainWindow::pGenSet(const int channel, const PulseGenerator::Setting s, const QVariant val)
{
	if(channel>-1)
	{
		QMetaObject::invokeMethod(hwm,"setPulse",Q_ARG(int,channel+1),
							 Q_ARG(PulseGenerator::Setting,s),Q_ARG(QVariant,val));
		return;
	}
}

void MainWindow::singleScanCallback()
{
	if(batchThread->isRunning())
		return;

	//launch single scan dialog...
	SingleScanDialog d(this);
	d.setWindowTitle(QString("Single Scan"));

    AutoFitWidget *aw = new AutoFitWidget(guessBufferString(),ui->peakUpPlot->getDelay(),ui->peakUpPlot->getHpf(),ui->peakUpPlot->getExp(),ui->peakUpPlot->getPadFidBox()->isChecked(),293.15,&d);
	d.insertAutoFitWidget(aw);

	SingleScanWidget *ssw = d.ssWidget();

	ssw->setFtmFreq(ui->ftmControlDoubleSpinBox->value());
	ssw->setAttn(ui->attnControlSpinBox->value());
	ssw->setDrFreq(ui->drControlDoubleSpinBox->value());
	ssw->setDrPower(ui->pwrControlDoubleSpinBox->value());
	ssw->setPulseConfig(readPulseConfig());
    ssw->setProtectionTime(ui->protectionSpinBox->value());
    ssw->setScopeTime(ui->scopeDelaySpinBox->value());
    ssw->setMagnet(ui->magnetOnOffButton->isChecked());
    ssw->enableSkipTune();

	int ret = d.exec();

	if(ret == QDialog::Rejected)
		return;

    Scan scan = ssw->toScan();
    AbstractFitter *af = aw->toFitter();

	BatchManager *bm = new BatchSingle(scan,af);

	makeBatchConnections(bm);

	bm->moveToThread(batchThread);
	batchThread->start();

	uiState = Acquiring;
	updateUiConfig();

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
	ssw->setPulseConfig(readPulseConfig());
    ssw->setProtectionTime(ui->protectionSpinBox->value());
    ssw->setScopeTime(ui->scopeDelaySpinBox->value());
    ssw->setMagnet(ui->magnetOnOffButton->isChecked());

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

	makeBatchConnections(bm,wiz.sleepWhenComplete());

	bm->moveToThread(batchThread);
	batchThread->start();

	uiState = Acquiring;
	updateUiConfig();

	delete ssw;
	delete aw;
}

void MainWindow::gasNamesChanged()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.setValue(QString("gas1Name"),ui->g1lineEdit->text());
    s.setValue(QString("gas2Name"),ui->g2lineEdit->text());
    s.setValue(QString("gas3Name"),ui->g3lineEdit->text());
    s.setValue(QString("gas4Name"),ui->g4lineEdit->text());
    s.sync();
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
		uiState = Asleep;
	else
		uiState = Idle;

	QMetaObject::invokeMethod(hwm,"sleep",Q_ARG(bool,b));
	updateUiConfig();
	if(b)
		QMessageBox::information(this,QString("Sleep Mode Active"),QString("Sleep mode is active! Toggle it off before proceeding. Also, don't forget to re-enable pressure control."),QMessageBox::Ok);

}

void MainWindow::hardwareStatusChanged(bool success)
{
    //show an error if we go from connected to disconnected
    if((hardwareConnected && !success) || (!hardwareConnected && !success))
    {
		statusLabel->setText(QString("A hardware error occurred. See log for details."));
        ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    }
    else if(!hardwareConnected && success)
    {
        statusLabel->setText(QString("Initialization complete."));
        setHardwareRanges();
    }

    hardwareConnected = success;
	updateUiConfig();
}

void MainWindow::launchCommunicationDialog()
{
	CommunicationDialog d(this);
	connect(&d,&CommunicationDialog::testConnection,hwm,&HardwareManager::testObjectConnection);
	connect(hwm,&HardwareManager::testComplete,&d,&CommunicationDialog::testComplete);

	d.exec();
}

void MainWindow::launchFtSettings()
{
	FtSynthSettingsWidget *w = new FtSynthSettingsWidget;

	//connect band change signal here
    connect(w,&SynthSettingsWidget::bandChanged,hwm,&HardwareManager::ftmSynthChangeBandFromUi);

	launchSettingsDialog(w);
    synthSettingsChanged();
}

void MainWindow::launchDrSettings()
{
	DrSynthSettingsWidget *d = new DrSynthSettingsWidget;

	//connect band change signal here
    connect(d,&SynthSettingsWidget::bandChanged,hwm,&HardwareManager::drSynthChangeBandFromUi);

    launchSettingsDialog(d);
    synthSettingsChanged();
}

void MainWindow::launchIOBoardSettings()
{
    IOBoardConfigDialog d(this);

    connect(&d,&IOBoardConfigDialog::testConnection,hwm,&HardwareManager::testObjectConnection);
    connect(hwm,&HardwareManager::testComplete,&d,&IOBoardConfigDialog::testComplete);

    if(d.exec() == QDialog::Accepted)
        QMetaObject::invokeMethod(hwm,"configureIOBoard");
}

void MainWindow::launchPulseGeneratorSettings()
{
    PulseGeneratorSettingsDialog d(this);

    if(d.exec() == QDialog::Accepted)
    {
        QMetaObject::invokeMethod(hwm,"applyPGenSettings");
        applyPulseNames();
    }
}

void MainWindow::applyPulseNames()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("pulseGenerator"));
    ui->chELabel->setText(s.value(QString("chEName"),QString("Aux1")).toString());
    ui->chFLabel->setText(s.value(QString("chFName"),QString("Aux2")).toString());
    ui->chGLabel->setText(s.value(QString("chGName"),QString("Aux3")).toString());
    ui->chHLabel->setText(s.value(QString("chHName"),QString("Aux4")).toString());
    ui->chELedLabel->setText(s.value(QString("chEName"),QString("Aux1")).toString());
    ui->chFLedLabel->setText(s.value(QString("chFName"),QString("Aux2")).toString());
    ui->chGLedLabel->setText(s.value(QString("chGName"),QString("Aux3")).toString());
    ui->chHLedLabel->setText(s.value(QString("chHName"),QString("Aux4")).toString());
    s.endGroup();
}

void MainWindow::resolutionChanged(Oscilloscope::Resolution res)
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.setValue(QString("scope/resolution"),(int)res);
    s.sync();

    QMetaObject::invokeMethod(hwm,"scopeResolutionChanged");

}

void MainWindow::tuningComplete()
{
    if(uiState & Tuning)
        uiState ^= Tuning;

    if(mirrorProgress->minimum() == 0)
    {
        mirrorProgress->setRange(-100000,100000);
        mirrorProgress->setValue(0);
    }
    updateUiConfig();
}

void MainWindow::tuneCavityCallback()
{
    double freq = ui->ftmControlDoubleSpinBox->value();
    uiState |= Tuning;
    updateUiConfig();
    QMetaObject::invokeMethod(hwm,"tuneCavity",Q_ARG(double,freq));
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
        uiState |= Tuning;
        updateUiConfig();
        QMetaObject::invokeMethod(hwm,"calibrateCavity");
    }
}

void MainWindow::modeChanged(int mode)
{
    ui->actionTune_Up->setText(QString("Tune Up (mode %1)").arg(mode+1));
    ui->actionTune_Down->setText(QString("Tune Down (mode %1)").arg(mode-1));
}

void MainWindow::tuneUpCallback()
{
    if(uiState==Idle)
    {
        uiState |= Tuning;
        updateUiConfig();
        QMetaObject::invokeMethod(hwm,"changeCavityMode",Q_ARG(double,ui->ftmControlDoubleSpinBox->value()),Q_ARG(bool,true));
    }
}

void MainWindow::tuneDownCallback()
{
    if(uiState==Idle)
    {
        uiState |= Tuning;
        updateUiConfig();
        QMetaObject::invokeMethod(hwm,"changeCavityMode",Q_ARG(double,ui->ftmControlDoubleSpinBox->value()),Q_ARG(bool,false));
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

    //ftmSynth
    ui->ftmControlDoubleSpinBox->blockSignals(true);
    ui->ftmControlDoubleSpinBox->setRange(s.value(QString("ftmSynth/min"),5000.0).toDouble(),s.value(QString("ftmSynth/max"),26490.0).toDouble());
    ui->ftmControlDoubleSpinBox->blockSignals(false);

    //attenuators
    ui->attnControlSpinBox->blockSignals(true);
    ui->attnControlSpinBox->setRange(s.value(QString("attn/min"),0).toInt(),s.value(QString("attn/max"),70).toInt());
    ui->attnControlSpinBox->blockSignals(false);

#ifndef CONFIG_NODRSYNTH
    //dr synth
    ui->drControlDoubleSpinBox->blockSignals(true);
    ui->pwrControlDoubleSpinBox->blockSignals(true);
    ui->drControlDoubleSpinBox->setRange(s.value(QString("drSynth/min"),50.0).toDouble(),s.value(QString("drSynth/max"),26500.0).toDouble());
    ui->pwrControlDoubleSpinBox->setRange(s.value(QString("drSynth/minPower"),-70.0).toDouble(),s.value(QString("drSynth/maxPower"),17.0).toDouble());
    ui->drControlDoubleSpinBox->blockSignals(false);
    ui->pwrControlDoubleSpinBox->blockSignals(false);
#endif

    //pulse generator
    double minWidth = s.value(QString("pulseGenerator/minWidth"),0.004).toDouble();
    double maxWidth = s.value(QString("pulseGenerator/maxWidth"),1.0e5).toDouble();
    double minDelay = s.value(QString("pulseGenerator/minDelay"),0.0).toDouble();
    double maxDelay = s.value(QString("pulseGenerator/maxDelay"),1.0e5).toDouble();

    for(int i=0;i<d_widthBoxes.size();i++)
    {
	   if(d_widthBoxes.at(i) != nullptr)
        {
		  QDoubleSpinBox *b = d_widthBoxes[i];
            b->blockSignals(true);
            b->setRange(minWidth,maxWidth);
            b->blockSignals(false);
        }
    }

    for(int i=0;i<d_delayBoxes.size();i++)
    {
	   if(d_delayBoxes.at(i) != nullptr)
        {
		  QDoubleSpinBox *b = d_delayBoxes[i];
            b->blockSignals(true);
            b->setRange(minDelay,maxDelay);
            b->blockSignals(false);
        }
    }

}

void MainWindow::changeAttnFileCallback()
{
    if(batchThread->isRunning())
        return;

    QDir d(QString("/home/data/QtFTM/tuningTables"));
    if(!d.exists())
        d.mkpath(d.absolutePath());

    QString attenFile = QFileDialog::getOpenFileName(this,QString("Select Attenuation Data File"),d.absolutePath(),QString("Attenuation Files (*.atn);;Text Files (*.txt);;All Files (*.*)"));

    if(!attenFile.isEmpty())
    {
	   connect(hwm,&HardwareManager::attnLoadSuccess,this,&MainWindow::attnFileSuccess,Qt::UniqueConnection);
        QMetaObject::invokeMethod(hwm,"changeAttnFile",Q_ARG(QString,attenFile));
    }

}

void MainWindow::attnFileSuccess(bool success)
{
    if(success)
        statusLabel->setText(QString("Attenuation file loaded successfully."));
    else
    {
        statusLabel->setText(QString("Attenuation file could not be loaded. See log for details."));
        ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    }

    disconnect(hwm,&HardwareManager::attnLoadSuccess,this,&MainWindow::attnFileSuccess);
}

void MainWindow::genAttnTableCallback()
{
    uiState = Tuning;
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

    QMetaObject::invokeMethod(hwm,"prepareForAttnTableGeneration",Q_ARG(int,attn10GHzBox->value()));
}

void MainWindow::attnTablePrepComplete(bool success)
{
    if(success)
        statusLabel->setText(QString("Preparation for generating attenuation table completed successfully"));
    else
    {
        statusLabel->setText(QString("A problem occurred while preparing to generate attenuation table. See log for details."));
        uiState = Idle;
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
        uiState = Idle;
        updateUiConfig();
        return;
    }

    Scan scan;
    scan.setTargetShots(0);
    scan.setFtFreq(10030.0);
    scan.setAttenuation(ui->attenuationSpinBox->value());
    scan.setDrFreq(ui->drControlDoubleSpinBox->value());
    scan.setDrPower(ui->powerDoubleSpinBox->value());
    scan.setPulseConfiguration(readPulseConfig());
    scan.setProtectionDelayTime(ui->protectionSpinBox->value());
    scan.setScopeDelayTime(ui->scopeDelaySpinBox->value());

    BatchAttenuation *bm = new BatchAttenuation(minFreqBox->value(),maxFreqBox->value(),stepBox->value(),scan.attenuation(),scan,nameBox->text());

    //connections for this one are a little bit different...
    ui->actionPrint_Summary->setEnabled(false);
    connect(batchThread,&QThread::started,bm,&BatchManager::beginBatch);
    connect(bm,&BatchManager::titleReady,ui->batchPlot,&BatchPlot::setBatchTitle);
    connect(bm,&BatchManager::batchComplete,this,&MainWindow::attnTableBatchComplete);
    connect(bm,&BatchManager::batchComplete,bm,&QObject::deleteLater);
    connect(bm,&BatchManager::batchComplete,ui->batchPlot,&BatchPlot::enableReplotting);
    connect(bm,&BatchManager::fatalSaveError,this,&MainWindow::fatalSaveError);
    connect(bm,&QObject::destroyed,batchThread,&QThread::quit);
    connect(bm,&BatchManager::beginScan,sm,&ScanManager::prepareScan);
    connect(sm,&ScanManager::dummyComplete,bm,&BatchManager::scanComplete);
    connect(bm,&BatchAttenuation::elementComplete,this,&MainWindow::updateProgressBars);
    connect(ui->actionAbort,&QAction::triggered,bm,&BatchAttenuation::abort);
    connect(bm,&BatchManager::logMessage,lh,&LogHandler::logMessage);
    connect(bm,&BatchManager::plotData,ui->batchPlot,&BatchPlot::receiveData);

    ui->shotsProgressBar->setRange(0,0);
    ui->shotsProgressBar->setValue(0);
    ui->batchProgressBar->setValue(0);
    ui->batchProgressBar->setRange(0,bm->totalShots());

    ui->batchPlot->prepareForNewBatch(bm->type());
    if(ui->batchSplitter->sizes().at(1) == 0)
    {
        QList<int> sizes(ui->batchSplitter->sizes());
        sizes[0]-=250;
        sizes[1]+=250;
        ui->batchSplitter->setSizes(sizes);
    }

    ui->analysisWidget->plot()->clearRanges();

    uiState = Acquiring;
    updateUiConfig();

    bm->moveToThread(batchThread);
    batchThread->start();
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
    QMetaObject::invokeMethod(hwm,"changeAttnFile",Q_ARG(QString,attenFile));
    uiState = Idle;
    ui->actionPrint_Summary->setEnabled(true);
    updateUiConfig();


}

QList<PulseGenerator::PulseChannelConfiguration> MainWindow::readPulseConfig()
{
	QList<PulseGenerator::PulseChannelConfiguration> out;

	QMetaObject::invokeMethod(hwm,"readPGenAll",Qt::BlockingQueuedConnection,
						 Q_RETURN_ARG(QList<PulseGenerator::PulseChannelConfiguration>,out));
	return out;
}

void MainWindow::makeBatchConnections(BatchManager *bm, bool sleep)
{
	ui->actionPrint_Summary->setEnabled(false);
    connect(bm,&BatchManager::titleReady,ui->batchPlot,&BatchPlot::setBatchTitle);
    connect(bm,&BatchManager::processingComplete,ui->analysisWidget,&AnalysisWidget::newScan);
    connect(bm,&BatchManager::processingComplete,ui->peakListWidget,&PeakListWidget::addScan);
	connect(bm,&BatchManager::batchComplete,this,&MainWindow::batchComplete);
	connect(bm,&BatchManager::batchComplete,bm,&QObject::deleteLater);
    connect(bm,&BatchManager::batchComplete,ui->batchPlot,&BatchPlot::enableReplotting);
	connect(bm,&BatchManager::fatalSaveError,this,&MainWindow::fatalSaveError);
	connect(bm,&QObject::destroyed,batchThread,&QThread::quit);
    connect(bm,&BatchManager::beginScan,this,&MainWindow::scanStarting);
    connect(bm,&BatchManager::beginScan,sm,&ScanManager::prepareScan);
    connect(sm,&ScanManager::acquisitionComplete,bm,&BatchManager::scanComplete);
	connect(bm,&BatchManager::logMessage,lh,&LogHandler::logMessage);
	connect(bm,&BatchManager::plotData,ui->batchPlot,&BatchPlot::receiveData);

    ui->batchProgressBar->setValue(0);
    ui->batchProgressBar->setRange(0,bm->totalShots());

    ui->batchPlot->prepareForNewBatch(bm->type());

    //Don't clear peak list widget!
//    ui->peakListWidget->clearAll();

	if(bm->type() != BatchManager::SingleScan)
	{
		if(ui->batchSplitter->sizes().at(1) == 0)
		{
			QList<int> sizes(ui->batchSplitter->sizes());
			sizes[0]-=250;
			sizes[1]+=250;
			ui->batchSplitter->setSizes(sizes);
		}
	}

	ui->analysisWidget->plot()->clearRanges();

	if(bm->type() == BatchManager::DrScan)
	{
		QList<QPair<double,double> > ranges = dynamic_cast<BatchDR*>(bm)->integrationRanges();
		ui->acqFtPlot->attachIntegrationRanges(ranges);
		ui->analysisWidget->plot()->attachIntegrationRanges(ranges);
		connect(batchThread,&QThread::finished,ui->acqFtPlot,&FtPlot::clearRanges);
	}

    connect(batchThread,&QThread::started,bm,&BatchManager::beginBatch);
    if(sleep)
	    connect(batchThread,&QThread::finished,ui->actionSleep_Mode,&QAction::trigger,Qt::UniqueConnection);
    else
	   disconnect(batchThread,&QThread::finished,ui->actionSleep_Mode,&QAction::trigger);

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
	if(ui->g1setPointBox->value() > max)
	{
		max = ui->g1setPointBox->value();
		out = ui->g1lineEdit->text();
	}
	if(ui->g2setPointBox->value() > max)
	{
		max = ui->g2setPointBox->value();
		out = ui->g2lineEdit->text();
	}
	if(ui->g3setPointBox->value() > max)
	{
		max = ui->g3setPointBox->value();
		out = ui->g3lineEdit->text();
	}
	if(ui->g4DoubleSpinBox->value() > max)
	{
		max = ui->g4setPointBox->value();
		out = ui->g4lineEdit->text();
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
