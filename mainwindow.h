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
#include "batchscanplot.h"
#include "batchattnplot.h"
#include "drplot.h"
#include "surveyplot.h"

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
    void scanStarting(Scan s, bool isCal);
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
    void mirrorPosUpdate(int pos);
    void flowControllerUpdate(int i ,double d);
    void pressureUpdate(double p);
    void flowSetpointUpdate(int i, double d);
    void pressureSetpointUpdate(double d);
    void pressureControlMode(bool on);

	void singleScanCallback();
	void batchScanCallback();
	void sleep(bool b);
	void hardwareStatusChanged(bool success);
	void launchCommunicationDialog();
	void launchFtSettings();
	void launchDrSettings();
    void launchIOBoardSettings();
    void resolutionChanged(QtFTM::ScopeResolution res);
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
    void setLogIcon(QtFTM::LogMessageCode c);
    void updatePulseLeds(const PulseGenConfig cc);
    void updatePulseLed(int index, QtFTM::PulseSetting s, QVariant val);
    void dcVoltageUpdate(int v);

signals:
    void changeGasName(int, QString);
    void setFlowSetpoint(int i, double d);
    void pGenSettingUpdate(const int ch, const QtFTM::PulseSetting s, const QVariant val);
    void scopeResolutionChanged();

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

	QList<QPair<QLabel*,Led*>> d_ledList;
    QList<QLineEdit*> d_gasBoxes;
    QList<QDoubleSpinBox*> d_gasSetpointBoxes;

    QAction *res1kHzAction;
    QAction *res2kHzAction;
    QAction *res5kHzAction;
    QAction *res10kHzAction;

	State d_uiState;
	bool d_hardwareConnected;
    bool noHardwareMode;
    int d_logCount;
    QtFTM::LogMessageCode d_logIcon;

    void startBatchManager(BatchManager *bm);
	void launchSettingsDialog(SettingsWidget *w);
	QString guessBufferString();

protected:
	void closeEvent(QCloseEvent *ev);

signals:
	void closing();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MainWindow::State)

#endif // MAINWINDOW_H
