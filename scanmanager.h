#ifndef SCANMANAGER_H
#define SCANMANAGER_H

#include <QObject>
#include "datastructs.h"
#include "fid.h"
#include "oscilloscope.h"
#include "scan.h"

/*!
 \brief Class that handles data acquisition for scans

 This class takes data from the oscilloscope, averages it, and associates it as appropriate with scans.
 FIDs are received in the fidReceived() slot in the form of a raw QByteArray, and are converted into an Fid object.
 Once conversion is complete, the newFid() signal is emitted.
 This signal is always connected to the peakUpAverage() slot, which computes a rolling average of FIDs (d_peakUpfid) that is displayed on the hardware control tab, and that is also used to set integration ranges for a DR scan.
 The number of FIDs in the rolling average (d_peakUpAvgs) can be changed from the UI or programatically through the setPeakUpAvgs() slot.
 The rolling average is reset any time the probe frequency changes (probeFreqChanged() slot), and can be reset on demand, as well, with resetPeakUpAvgs().
 Each time the rolling average is updated, the new FID is sent to the UI with the peakUpFid() signal.

 When a Scan is received at the prepareScan() slot, an acquisition is initiated.
 First, checks are made to ensure that no other Scan is ongoing, and the Scan is then sent to the HardwareManager for initialization (initializeHardwareForScan() signal).
 After initialization in HardwareManager, the Scan is received at the startScan() slot.
 If all initialization was successful, averaging is initiated by connecting the newFid() signal to the acqAverage() slot, where FIDs are averaged until the target number of shots is reached or the scan is aborted.
 If the scan is paused, new FIDs are ignored in the acqAverage() function until unpaused.
 When a shot is averaged in, the scanShotAcquired() signal is emitted to increment the progress bar on the UI, and the scanFid() signal is emitted to update the plots on the UI.

 When a scan is complete, either by reaching the target number of shots or by being aborted, the Scan::save() function is called, and the scan is emitted (acquisitionComplete()) for further processing by the UI and the BatchManager.
 At this point, the newFid signal is disconnected from the acqAverage slot, and all acquisition-related variables are reset.

 Finally, some scans are used just to change hardware settings, not to actually start an acquisition.
 These are called dummy scans.
 For these, when the initialization is complete, the dummyComplete() signal is emitted; this is used for instance to initialize DR integration ranges.

*/
class ScanManager : public QObject
{
	Q_OBJECT
public:
	/*!
	 \brief Constructor

	 Connects newFid() signal to the peakUpAverage() slot, and initializes rolling average parameters

	 \param parent Is not set, as this lives in its own thread
	*/
	explicit ScanManager(QObject *parent = nullptr);
	
signals:
	/*!
	 \brief Sends message to log

	 \param QString The message
	 \param QtFTM::LogMessageCode Type of message
	*/
	void logMessage(const QString, const QtFTM::LogMessageCode = QtFTM::LogNormal);
	/*!
	 \brief Sends message to status bar.

	 Used to give status bar update with number of shots in a scan ("Acquiring (x/total)")

	 \param QString The message
	*/
	void statusMessage(const QString);
	/*!
	 \brief Sends the current rolling average Fid

	 This is connected to the plot on the instrument control tab, but is also connected to the plot when setting DR integration ranges.

	 \param Fid The rolling average
	*/
	void peakUpFid(const Fid);
	/*!
	 \brief Sends the Fid just received from the oscilloscope

	 When idle, this is only connected to the peakUpAverage() slot.
	 During an acquisition, it is also connected to the acqAverage() slot.

	 \sa fidReceived()
	 \sa d_peakUpFid

	 \param Fid
	*/
	void newFid(const Fid);
	/*!
	 \brief Signal sent to HardwareManager to prepare hardware for scan

	 \param Scan The scan
	*/
	void initializeHardwareForScan(Scan);
	/*!
	 \brief Emitted when hardware initialization is successful

	 This signal is used to increment the scan number on the UI left-hand display panel.
	 It ensures that the number displayed there always corresponds to the current or most recently completed scan.

	*/
	void initializationComplete();
	/*!
	 \brief Sends the current average Fid during an acquisition.

	 During an acquisition, this Fid is displayed on the right-hand plot on the display tab, and will also be displayed on the left plot if the current scan is selected.

	 \param Fid The averaged Fid
	*/
	void scanFid(const Fid);
	/*!
	 \brief Emitted when a shot is acquired during a scan

	 Used to update the scan and batch progress bars

	*/
	void scanShotAcquired();
	/*!
	 \brief Emitted when an acquisition is complete

	 This is emitted anytime a scan is complete (either by reaching target number of shots, failure, or aborting).
	 The only exception to this is a dummy scan.

	 \sa Scan::isDummy()

	 \param Scan
	*/
	void acquisitionComplete(Scan);
	/*!
	 \brief Emitted when hardware settings are made for a dummy scan

	 \sa Scan::isDummy()

	*/
    void dummyComplete(Scan);
	/*!
	 \brief Emitted when a scan save fails.

	 This should never happen, but if it does, this signal will activate the log tab on the UI and decrement the scan number displayed on the UI.

	*/
	void fatalSaveError();
	
public slots:
	/*!
	 \brief Converts raw oscilloscope response to Fid

	 Uses the current probe frequency and the Oscilloscope::parseWaveform() function to construct the Fid object.
	 Emits the newFid() signal.

	 \param d The oscilloscope response.
	*/
	void fidReceived(const QByteArray d);
	/*!
	 \brief Begins hardware initialization for scan.

	 Emits initializeHardwareForScan(), and sets waitingForInitialization to true

	 \param s The scan to initialize
	*/
	void prepareScan(Scan s);
	/*!
	 \brief Begins scan after hardware intialization

	 First, this function makes sure that the initialization was completed successfully and finds out if the scan is dummy.
	 In any case, sets waitingForInitialization to false, and if all initialization was successful, connects newFid() to acqAverage().
	 Sets currentScan to s.

	 \param s The initialized scan.
	*/
	void startScan(Scan s);
	/*!
	 \brief Folds an Fid into the rolling average (d_peakUpFid)

	 The algorithm used is to multiply the rolling average by the number of shots that have been previously averaged, add in the new Fid, and divide by the new total.
	 If the number of averages (d_peakUpCount) is already equal to or greater than the requested number (d_peakUpAvgs), it is set to d_peakUpAvgs-1 prior to the averaging procedure.

	 \param f The new Fid to average
	*/
	void peakUpAverage(const Fid f);
	/*!
	 \brief Sets the number of FIDs to include in the rolling average

	 The value of a is stored in settings

     \param a The new number of averages
	*/
	void setPeakUpAvgs(int a);
	/*!
	 \brief Resets rolling average by setting d_peakUpCount to 0

	*/
	void resetPeakUpAvgs(){ d_peakUpCount = 0; }
	/*!
	 \brief Folds an Fid into the average for the scan (currentScan)

	 If paused is true, the Fid is ignored.
	 Otherwise, the shot number is incremented, and the newFid signal disconnected from this slot if the target number has been reached.

	 \param f The new Fid to average
	*/
	void acqAverage(const Fid f);
	/*!
	 \brief Pauses acquisition by setting paused to true

	*/
    void pause() { d_paused = true; }
	/*!
	 \brief Resumes acquisition by setting paused to false.

	*/
    void resume() { d_paused = false; }
	/*!
	 \brief Aborts the current scan

	 Resets all scan-ralated variables, disconnects newFid() from acqAverage(), saves currentScan, and emits acquisitionComplete().

	*/
	void abortScan();
	/*!
	 \brief Records the current probe frequency (currentProbeFreq), which is needed to construct Fid from oscilloscope response

	 \param f The new probe frequency
	*/
    void setCurrentProbeFreq(double f){ d_currentProbeFreq = f; }

    void failure();

    void retryScan();

private:
	Fid d_peakUpFid; /*!< The Fid containing the rolling average */

    Scan d_currentScan; /*!< The ongoing or most recently-completed scan */
    Scan d_scanCopyForRetry;
    bool d_paused; /*!< Used to ignore new FIDs during an acquisition */
    bool d_acquiring;
    bool d_waitingForInitialization; /*!< True for the time between when a scan is sent to HardwareManager for initialization and when it is received in startScan() */
    bool d_connectAcqAverageAfterNextFid;

	int d_peakUpAvgs; /*!< Number of FIDs to include in d_peakUpFid */
	int d_peakUpCount; /*!< Current number of FIDs included in d_peakUpFid */
    double d_currentProbeFreq; /*!< Current probe frequency */
	
};

#endif // SCANMANAGER_H
