#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include "hardwaremanager.h"
#include "loghandler.h"
#include "scanmanager.h"
#include "batchmanager.h"
#include "settingswidget.h"
#include "led.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	enum StateFlag {
		Idle = 0x00,
		Acquiring = 0x01,
		Tuning = 0x02,
		Paused = 0x04,
		Asleep = 0x08
	};


	Q_DECLARE_FLAGS(State,StateFlag)

public slots:
	void updateProgressBars();
	void updateUiConfig();
    void saveLogCallback();
	void scanStarting(Scan s);
	void batchComplete(bool aborted);
	void pauseAcq();
	void resumeAcq();
    void synthSettingsChanged();
	void ftmCavityUpdate(double f);
	void ftmProbeUpdate(double f);
	void drSynthFreqUpdate(double f);
	void drSynthPwrUpdate(double p);
	void attnUpdate(int a);
    void taattnUpdate(int a);
    void setcvUpdate(int a);
    void magnetUpdate(bool mag);
    void viewBatchCallback();
    void updateScopeDelayOnUI(int a);
    void updateProtectionDelayOnUI(int a);
    void mirrorPosUpdate(int pos);
    void flowControllerUpdate(FlowController::FlowIndex i ,double d);
    void flowSetPointUpdated(FlowController::FlowIndex i, double d);
    void setFlowSetPoint(FlowController::FlowIndex i, double d);
    void pressureControlMode(bool on);
	void fatalSaveError();

	void pGenSettingUpdate(const int ch, const PulseGenerator::Setting s, const QVariant val);
	void pGenChannelUpdate(const PulseGenerator::PulseChannelConfiguration p);
	void pGenAllUpdate(const QList<PulseGenerator::PulseChannelConfiguration> l);

	void pGenSet(const int channel, const PulseGenerator::Setting s, const QVariant val);

	void singleScanCallback();
	void batchScanCallback();
    void gasNamesChanged();
	void sleep(bool b);
	void hardwareStatusChanged(bool success);
	void launchCommunicationDialog();
	void launchFtSettings();
	void launchDrSettings();
    void launchIOBoardSettings();
    void launchPulseGeneratorSettings();
    void applyPulseNames();
    void resolutionChanged(Oscilloscope::Resolution res);
    void tuningComplete();
    void tuneCavityCallback();
    void calibrateCavityCallback();
    void modeChanged(int mode);
    void tuneUpCallback();
    void tuneDownCallback();
    void tuningVoltageChanged(int v);
    void setHardwareRanges();
    void changeAttnFileCallback();
    void attnFileSuccess(bool success);
    void genAttnTableCallback();
    void attnTablePrepComplete(bool success);
    void attnTableBatchComplete(bool aborted);

private:
	Ui::MainWindow *ui;
	QThread *acquisitionThread;
	QThread *controlThread;
	QThread *batchThread;
	QLabel *statusLabel;
	QProgressBar *mirrorProgress;

	ScanManager *sm;
	HardwareManager *hwm;
	LogHandler *lh;

	QList<PulseGenerator::PulseChannelConfiguration> readPulseConfig();
	QList<QDoubleSpinBox*> d_delayBoxes;
	QList<QDoubleSpinBox*> d_widthBoxes;
	QList<QPushButton*> d_onOffButtons;
	QList<Led*> d_leds;

    QAction *res1kHzAction;
    QAction *res2kHzAction;
    QAction *res5kHzAction;
    QAction *res10kHzAction;

	State uiState;
	bool hardwareConnected;
    bool noHardwareMode;

    void makeBatchConnections(BatchManager *bm, bool sleep = false);
	void launchSettingsDialog(SettingsWidget *w);
	QString guessBufferString();

protected:
	void closeEvent(QCloseEvent *ev);

signals:
	void closing();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MainWindow::State)

#endif // MAINWINDOW_H
