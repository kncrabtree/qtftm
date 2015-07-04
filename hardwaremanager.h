#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <QObject>
#include "datastructs.h"
#include "oscilloscope.h"
#include "gpibcontroller.h"
#include "ftmsynthesizer.h"
#include "drsynthesizer.h"
#include "flowcontroller.h"
#include "pulsegenerator.h"
#include "scan.h"
#include "attenuator.h"
#include "motordriver.h"
#include "pinswitchdrivedelaygenerator.h"
#include <QThread>
#include "ioboard.h"

/*!
 * \brief Master control class for interacting with instrument hardware
 *
 * The HardwareManager object is the gatekeeper for communicating with instrument hardware.
 * Any time the program needs to talk to a piece of hardware, it must go through this object.
 * Conversely, if instrument hardware needs to interact with the user interface, it must also go through this object.
 * The HardwareManager also handles the sequential operations needed to initialize scans, tune the cavity, etc.
 *
 * This object is designed to live in its own thread, and it contains additional threads for each piece of hardware directly attached (see note below on GPIB devices).
 * To allow smooth operation without UI hitches and without missing hardware signals, signals and slots are used to interface with this object and with the hardware.
 * Most of the declared signals and slots are used to pass along information to the UI, or to perform special functions for each piece of hardware.
 * Those will not be covered in much detail here; see the appropriate HardwareObject subclasses for further information.
 *
 * However, the HardwareManager does have some internal functions that are used to make sure the instrument is in a consistent state.
 * Once the HardwareManager is created and its signals connected, it is pushed into its own thread, and the thread's started() signal is connected to the initializeHardware() slot.
 * This is where the individual HardwareObjects are created, and their respective signal and slot connections made.
 * Each object that is created should be put into its own thread.
 * The HardwareManager keeps a list of the devices (d_hardware), and looks at their subclass information to generate appropriate lists in the settings so that the communication settings dialog can put the devices into the correct categories.
 * The d_hardware list is used to initiate testConnection() calls from the communication dialog, and to do common operations to all devices (e.g. sleep).
 * As these pieces of hardware come online, their connectionResult() signals are attached to the connectionResult() slot, which keeps track of whether communication is successful in d_status (see checkStatus()).
 * Once they are all successfully tested, the allHardwareConnected() signal is used to configure the UI.
 * In the event of a hardware failure, the hardwareFailure() slot is invoked, and the UI is disabled until a successful connection is re-established.
 *
 * GPIB instruments are handled a little differently than others.
 * Communication with GPIB devices must be routed through the GpibLanController, which is itself a TcpInstrument.
 * Because of this, the GPIB devices are not directly known to the HardwareManager; they are owned by the GpibLanController.
 * When the controller is initialized, it will initialize each of those instruments in turn, but the HardwareManager needs to know how many to expect so that it knows when to emit the allHardwareConnected() signal.
 * The number is stored in d_numGpib, and is set when the GpibLanController emits its numInstruments() signal during its initialize() call.
 * All subsequent GPIB commands and responses go through the controller, which itself contains a QMutex to prevent concurrent access to multiple devices on the bus.
 *
 * Assuming all hardware is operational, the HardwareManager will pass along control commands from the user interface to the appropriate hardware through queued function calls with QMetaObject::invokeMethod(), and the hardware status is relayed back to the UI with the corresponding signals.
 * Also, any time the oscilloscope completes an acquisition, the HardwareManager passes along the information to the ScanManager object, which handles processing and averaging as needed.
 * It is important to ensure that function calls in HardwareManager return quickly unless scope triggering is disabled (see pauseScope()), otherwise the event queue may start to fill up and FID processing might be delayed, ultimately resulting in their being associated with the wrong acquisition.
 *
 * When an acquisition (scan) is starting, the ScanManager sends the Scan to the HardwareManager for initialization (prepareForScan()).
 * The first step is to complete cavity tuning if necessary.
 * Once cavity tuning is finished, the motor driver's tuningComplete() signal invokes the finishPreparation() slow, which applies the settings indicated in the Scan object, and stores the actual settings as read from the hardware.
 * Once all settings are made, the Scan::initilizationComplete() function is called to indicate that everything is ready to go, and it is returned to the ScanManager via the scanInitialized() signal.
 * Note that in order to read the actual settings from the hardware, the communication occurs through a BlockingQueuedConnection.
 * There are convenience functions for this so that the pieces of code in the intitialization functions are more readable.
 *
 * Cavity tuning requires some special discussion.
 * Since it can be a slow process requiring several seconds, it is critical that the motor driver is in its own thread so that hardware configuration changes can be propagated to the UI.
 * When tuning the cavity, the FTM synthesizer is set to the target cavity frequency; the attentuation is set to the appropriate value for tuning; all pulses except gas, MW, and Mon are disabled; and the IOBoard is set to assert CW mode.
 * Once all this is complete, the MotorDriver::tune() function is called through a queued connection.
 * If tuning is requested from outside an acquisition, the FTM synthesizer is set to the probe frequency, and the attenuation and pulse generator settings restored.
 * However, after a calibration, the attenuation is not restored to its original value, and the FTM synthesizer stays at the calibration probe frequency (otherwise, the process is the same).
 *
 * There is also a routine for cavity calibration and tuning that takes place prior to generating a tuning attenuation table.
 * This starts with the function prepareForAttnTableGeneration(), which puts the instrument into tuning mode and disconnects .
 * The prepareForAttnTableGeneration() function invokes MotorDriver::calibrate(), and the MotorDriver::tuningComplete() signal is connected to the attnTablePrepTuningComplete() slot, which determines whether the attenuation needs to be changed in order to get a 1 V peak tuning signal.
 * Additional calls to MotorDriver::tune() are then made as needed to ensure the correct attenuation is calculated.
 * Once that is achieved, the cavity is recalibrated, and the MotorDriver::tuningComplete() signal is reconnected to cavityTuneComplete().
 *
 */
class HardwareManager : public QObject
{
	Q_OBJECT
public:
    /*!
     * \brief Constructor. Initializes some simple variables
     * \param parent Should be nullptr
     */
	explicit HardwareManager(QObject *parent = nullptr);

    /*!
     * \brief Destructor.
     *
     * Stops all threads, and deletes objects that live in them.
     */
	~HardwareManager();

signals:
    /*!
     * \brief Emitted when initialization of a scan is complete (whether or not initialization was successful)
     * \param s The Scan that was initialized.
     */
	void scanInitialized(Scan s);

    /*!
     * \brief Emitted when cavity frequency changes
     * \param f New cavity frequency
     */
	void ftmSynthUpdate(const double f);

    /*!
     * \brief Triggered from UI when band is changed manually
     */
    void ftmSynthChangeBandFromUi();

    /*!
     * \brief Emitted when FT synthesizer frequency changes, but not the target cavity frequency
     * \param f New synth probe frequency
     */
	void probeFreqUpdate(const double f);

    /*!
     * \brief Emitted when a synthesizer band (either DR or FTM) is automatically changed
     */
    void synthRangeChanged();

    /*!
     * \brief Emitted when DR frequency changes
     * \param f New DR frequency
     */
	void drSynthFreqUpdate(const double f);

    /*!
     * \brief Emitted when DR synth power changes
     * \param p New power
     */
	void drSynthPwrUpdate(const double p);

    /*!
     * \brief Triggered from UI when band is changed manually
     */
    void drSynthChangeBandFromUi();

    /*!
     * \brief Emitted when attenuation is changed
     * \param a New attenuation
     */
	void attenUpdate(const int a);

    void taattenUpdate(const int a);

    /*!
     * \brief Emitted when one of the flow controller values changes
     * \param FlowController::FlowIndex Which channel changed (pressure; ch 1-4)
     * \param double The new value
     */
    void flowUpdate(int,double);

    void pressureUpdate(double);

    /*!
     * \brief Emitted when a setpoint changes
     * \param FlowController::FlowIndex Which channel changed (pressure; ch 1-4)
     * \param double The new value
     */
    void flowSetpointUpdate(int,double);

    void pressureSetpointUpdate(double);

    void setFlowSetpoint(int i, double val);

    void setPressureSetpoint(double);

    void setGasName(int, QString);

    /*!
	\brief Emitted to indicate whether the flow controller PID loop is active

	\param bool Whether PID mode is active
    */
    void pressureControlMode(bool);

    /*!
	\brief Signal passed to the flow controller to toggle PID loop

	\param bool If true, PID loop should be enabled
    */
    void setPressureControlMode(bool);

    /*!
     * \brief Emitted when the discharge voltage changes
     * \param New voltage
     *
     * \todo This is presently not implemented. If the voltage is ever to be set or read from the program, use this signal to pass the new value to the UI
     */
	void dcVoltageUpdate(const double v);

    /*!
     * \brief Emitted when the scope acquires a trace
     * \param QByteArray The raw waveform data
     */
	void scopeWaveAcquired(const QByteArray);

    /*!
     * \brief Emitted when mirror position changes
     * \param pos New encoder position
     */
    void mirrorPosUpdate(int pos);

    /*!
     * \brief Emitted when one setting for one channel on the pulse generator changes
     * \param int the channel number
     * \param PulseGenerator::Setting the setting that changed
     * \param QVariant The new value
     *
     * Note that the other to similar function just encapsulate this information for a single channel, or for all channels
     */
	void pGenChannelSetting(int,QtFTM::PulseSetting,QVariant);

    /*!
     * \brief Emitted when multiple channel settings change
     * \sa pGenChannelSetting()
     */
	void pGenConfigUpdate(const PulseGenConfig);

    /*!
     * \brief Emitted when a message needs to be displayed on the log
     * \param QString Message
     * \param QtFTM::LogMessageCode Message status
     */
	void logMessage(const QString, const QtFTM::LogMessageCode = QtFTM::LogNormal);

    /*!
     * \brief Emitted when a message needs to be displayed in the status bar
     * \param QString The message
     */
	void statusMessage(const QString);

    /*!
     * \brief Emitted when hardware comes online or goes offline
     * \param bool Whether all known hardware has connected successfully
     */
	void allHardwareConnected(bool);

    /*!
     * \brief Emitted when a connection is being tested from the communication dialog
     * \param QString The HardwareObject key
     * \param bool Whether connection was successful
     * \param QString Error message
     */
	void testComplete(QString,bool,QString);

    /*!
     * \brief Emitted when cavity tuning finishes
     */
    void tuningComplete();

    /*!
     * \brief Emitted when cavity modes are calculated to indicate if a higher mode number is accessible
     * \param bool True if mode+1 is also in the range of the cavity
     */
    void canTuneUp(bool);

    /*!
     * \brief Emitted when cavity modes are calculated to indicate if a lower mode number is accessible
     * \param bool True if mode-1 is also in the range of the cavity
     */
    void canTuneDown(bool);

    /*!
     * \brief Emitted when the cavity mode nubmer changes
     * \param int New mode number
     */
    void modeChanged(int);

    /*!
	\brief Emitted when tuning is completed

	\param int The peak voltage during tuning, in mV
    */
    void tuningVoltageChanged(int);

    /*!
	\brief Emitted when attenuator finishes parsing an attenuation table

	\param bool Whether parsing was successful
    */
    void attnLoadSuccess(bool);

    /*!
	\brief Emitted after instrument finishes preparation for generating an attenuation table

	\param bool Whether the cavity calibration sequence was completed successfully
    */
    void attnTablePrepComplete(bool);

    //UI Control callbacks
    /*!
     * \brief Invoked to set cavity frequency when user changes it in the control box
     * \param f New frequency
     */
    void setFtmCavityFreqFromUI(double f);

    /*!
	* \brief Sets attenuation from UI control box
	* \param a New attenuation
	*/
	void setAttnFromUI(int a);

    /*!
     * \brief Invoked to set DR frequency from UI control box
     * \param f New DR frequency
     */
    void setDrSynthFreqFromUI(double f);

    /*!
     * \brief Invoked to set DR synth power from UI control box
     * \param p New power
     */
    void setDrSynthPwrFromUI(double p);

    /*!
	* \brief Sets a single setting for a pulse generator channel
	* \param ch Channel number
	* \param s Setting to change
	* \param x New value
	*/
	void setPulseSetting(const int ch, const QtFTM::PulseSetting s, const QVariant x);

	void setRepRate(double);
	void repRateUpdate(double);


    /*!
     * \brief Sets protection delay from UI control box
     * \param a New attenuation
     */
    void setProtectionDelayFromUI(int a);

    /*!
     *\brief Tells pin switch driver to load a new protection delay
    */

    void setScopeDelayFromUI(int a);

    void hmProtectionDelayUpdate(int a);

    void hmScopeDelayUpdate(int a);

    void failure();

    void setMagnetFromUI(bool);
    void magnetUpdate(bool);
    void retryScan();

	
public slots:
    /*!
     * \brief Creates all HardwareObjects and generates lists
     *
     * Called when thread begins.
     */
	void initializeHardware();

    /*!
     * \brief Records whether hardware connection was successful
     * \param obj A HardwareObject that was tested
     * \param success Whether communication was sucessful
     * \param msg Error message
     */
	void connectionResult(HardwareObject *obj, bool success, QString msg);

    /*!
     * \brief Calls the appropriate object's testConenction() slot
     * \param type Superclass of object. Used to identify GpibInstruments
     * \param key The HardwareObject key to be tested
     */
	void testObjectConnection(QString type, QString key);

    /*!
     * \brief Sets hardware status in d_status to false, disables program
     * \param obj The object that failed.
     */
	void hardwareFailure(HardwareObject *obj);

    /*!
     * \brief Stores the number of instruments connected to the GPIB-LAN controller
     * \param i Number of instruments
     */
    void setNumGpibInstruments(int i){ d_numGpib = i; }



    /*!
    * \param fileName The full path of the file to be loaded
    */
    void changeAttnFile(QString fileName);

    /*!
     * \brief Disables all pulsed except microwave, gas, and monitor
     * \return Pulse channel configuration PRIOR to disabling channels (so that settings can be restored afterwards)
     */
    PulseGenConfig configurePGenForTuning();

	//functions to be used during a scan; these return (as appropriate) the actual settings that will be saved!
	//these functions block until the operation is complete, so don't use them from the UI!

    /*!
     * \brief Sets all channels settings for pulse generator
     * \param l List of settings
     * \return The actual settings after they have been applied
     */
	PulseGenConfig setPulseConfig(const PulseGenConfig c);

    /*!
     * \brief Sets FTM synthesizer to the probe frequency for the current cavity frequency
     * \return The actual probe frequency setting
     */
	double goToFtmSynthProbeFreq();

    /*!
     * \brief Sets synthesizer to current target cavity frequency (for tuning)
     * \return Whether setting was successful
     */
	bool goToFtmSynthCavityFreq();

    /*!
     * \brief Sets the target cavity frequency, but does NOT actually change the frequency!
     *
     * The main use for this function is to allow the synthesizer to determine whether the given frequency is accessible in the current band.
     * If autoswitching is enabled, it will change bands as appropriate
     *
     * \param d New target cavity frequency
     * \return Whether setting can be reached in current band (after autoswitching, if enabled)
     */
    bool setFtmSynthCavityFreq(double d);

    /*!
	\brief Fetches most recent tuning voltage from the MotorDriver

	\return int Tuning voltage, in mV
    */
    int readCavityTuningVoltage();

    /*!
	\brief Fetches most recent tuning attenuation from MotorDriver

	\return int Tuning attenuation, in dB
    */
    int readTuneAttenuation();

    /*!
    \brief Fetches most recent protection delay PinSwitchDriver

    \return int in microseconds
    */

    int setProtectionDelay(int a);
    /*!
    \brief Fetches most recent scope delay from PinSwitchDriver

    \return int in microseconds
    */
    int setScopeDelay(int a);

    /*!
	\brief Reads most recent calibration voltage from MotorDriver

	\return int Calibration voltage, in mV
    */
    int readCalVoltage();

    /*!
     * \brief Sets DR synthesizer frequency
     * \param f New frequency
     * \return Actual frequency setting
     */
	double setDrSynthFreq(double f);

    /*!
     * \brief Sets power of DR synthesizer
     * \param p New power
     * \return Actual synthesizer power setting
     */
	double setDrSynthPwr(double p);

    /*!
     * \brief Sets the IO Board's cw line for tuning more
     * \param cw Whether tuning mode is enabled
     * \return 0 for tuning mode off, 1 for tuning mode on, -1 for error
     */
    int setCwMode(bool cw);

    int setMagnetMode(bool mag);

    /*!
	\brief Causes Oscilloscope to ignore trigger events

	\param pause If true, triggers will be ignored
    */
    void pauseScope(bool pause);

    /*!
     * \brief Reads all gas flows and the pressure from flow controller
     * \return Structure containing readings
     */
    const FlowConfig readFlowConfig();

    /*!
     * \brief Starts the cavity tuning process for a scan, if needed
     *
     * First, d_waitingForScanTune is set to true.
     * Then this function checks to see if tuning is necessary (MotorDriver::canSkipTune()), and stores a copy of the scan object for later use.
     * If tuning is needed, tuneCavity() is called.
     * Otherwise, this function sets the target cavity frequency and directly calls finishPreparation()
     *
     * \param s The scan to prepare
     */
    void prepareForScan(Scan s);

    /*!
     * \brief Applies scan settings if tuning was successful.
     *
     * Sets d_waitingForScanTune to false.
     * If tuneSuccess is false this function returns immediately, emitting the scanInitialized() signal without calling Scan::initializationComplete()
     * Otherwise, settings are made in the following order: Attenuation, FT synth probe frequency, dr synth frequency, dr synth power, and pulse generator configuration.
     * If any of these settings fails, this function returns immediately as indicated earlier.
     * Otherwise, flows are read, Scan::initializationComplete() is called, and then the scanInitialized() signal is emitted.
     *
     * \param tuneSuccess Whether tuning was successful
     */
    void finishPreparation(bool tuneSuccess);

    /*!
     * \brief Calls HardwareObject::sleep() for all attached devices
     * \param b True if hardware should sleep. False if hardware should wake up.
     */
	void sleep(bool b);

    /*!
     * \brief Invokes Oscilloscope::setResolution() through a queued connection.
     *
     * This function is called when a user changes the requested resolution.
     * Technically, this could have been declared as a signal, and been connected to the scope's setResolution slot
     */
    void scopeResolutionChanged();

    /*!
     * \brief Prepares hardware for tuning mode, and begins the cavity tune.
     *
     * First, this function records the current attenuation and pulse channel configuration.
     * Then, the FTM synthesizer target frequency is changed, and the synth is set to the cavity frequency.
     * The attenuation is set to the appropriate level for tuning, the pulse generator is configured for tune mode, and the IO board is placed into cw Mode.
     * If any of these settings fail, original values are restored to all, and the function returns immediately, calling finishPreparation(false) if d_waitingForScanTune is true.
     * Once settings are made, MotorDriver::tune is called through a queued connection, and the function returns while that is ongoing.
     * If the mode number is specified, that is passed along to the tune function as well.
     *
     * \param freq Cavity frequency
     * \param mode Mode number to tune to (-1 for default mode)
     */
    void tuneCavity(double freq, int mode = -1, bool measureOnly = false);

    /*!
     * \brief Restores settings to pre-tune values, and proceeds with scan preparation if necessary.
     *
     * Depending on the status variables d_waitingForScanTune and d_waitingForCalibration, some settings are restored.
     * If either of those is true, the pulse generator settings are restored, but not the attenuation.
     * Otherwise, the attenuation is restored as well.
     * The FTM synth is set to the probe frequency, and d_waitingForCalibration is set to false.
     * A status message is emitted along with the tuningComplete() signal for the UI.
     * Finally, if d_waitingForScanTune is true, finishPreparation() is called.
     *
     * \param success Whether tuning was successful
     */
    void cavityTuneComplete(bool success);

    /*!
     * \brief Similar to tuneCavity, but the target frequency is set to 10030
     */
    void calibrateCavity();

    /*!
	\brief Tunes cavity to next mode

	\param freq Target cavity frequency
	\param above If true, tune to currentMode+1. Otherwise, currentMode-1
    */
    void changeCavityMode(double freq, bool above);

    /*!
	\brief Configures instrument for a sequence of calibrations

	The MotorDriver::tuningComplete() signal is connected to attnTablePrepTuneComplete(), the attenuator is set to a, and the system configured for tuning.

	\param a Initial attenuation, in dB
    */
    void prepareForAttnTableGeneration(int a = 20);

    /*!
	\brief Determines whether calibration sequence is successful

    After a calibration or tune is complete, the tuning voltage is dpd.
	If it's saturated (v>3000), the attenuation is increased by 10 dB and the cavity re-tuned.
	If the signal was too low (v<300), the attenuation is decreased by 5 dB and re-tuned.
	If rough tuning failed, then the sequence is aborted.
	Finally, if 300<=v<=3000, the correct attenuation to give 1000 mV is calculated, and the cavity re-calibrated.

	\param success Whether tuning was successful
    */
    void attnTablePrepTuneComplete(bool success);

    /*!
	\brief Takes instrument out of tuning mode, and restores original signal-slot connections

	\param success Whether calibration sequence was successful
    */
    void restoreSettingsAfterAttnPrep(bool success);

private:
	Oscilloscope *scope;
    GpibController *gpib;
	Attenuator *attn;
	MotorDriver *md;
    PinSwitchDriveDelayGenerator *pin;
    IOBoard *iob;
    FlowController *fc;
    PulseGenerator *pGen;
    FtmSynthesizer *p_ftmSynth;
    DrSynthesizer *p_drSynth;

    Scan d_currentScan;
    bool d_waitingForScanTune;
    bool d_waitingForCalibration;
    PulseGenConfig d_tuningOldPulseConfig;
    int d_tuningOldA;

	QHash<QString,bool> d_status;
    QList<QPair<HardwareObject*,QThread*>> d_hardwareList;
	void checkStatus();

    int d_numGpib;
    bool d_firstInitialization;

    bool canSkipTune(double f);
    void startTune(double freq, int attn, int mode = -1);
    int measureCavityVoltage();
    void startCalibration();
    void shutUpMotorDriver(bool quiet);
	
};

#endif // HARDWAREMANAGER_H
