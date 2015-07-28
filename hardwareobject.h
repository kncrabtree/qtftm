#ifndef HARDWAREOBJECT_H
#define HARDWAREOBJECT_H

#include <QObject>

#include <QString>
#include <QSettings>
#include <QApplication>

#include "datastructs.h"
#include "communicationprotocol.h"

/*!
 * \brief Abstract base class for all hardware connected to the instrument.
 *
 * This class establishes a common interface for all hardware.
 * Adding a new piece of hardware to the code involves creating a subclass of HardwareObject.
 * Each hardware object has a name (d_prettyName and name()) that is used in the user interface, a key (d_key and key()) that is used to refer to the object in the settings file, and a subKey (d_subKey and subKey()) that refer to the particular implementation of the hardware.
 * Subclasses must assign strings to these variables in their constructors.
 * The base class sets d_key, and the implementations set d_subKey and d_prettyName.
 * For communication with the UI, a logMessage() signal exists that can print a message in the log with an optional status code.
 * In general, this should only be used for reporting errors, but it is also useful in debugging (see QtFTM::LogDebug).
 *
 * Generally, a HardwareObject is created, signal-slot connections are made, and then the object may optionally pushed into its own thread.
 * The thread's started() signal is connected to the initialize() pure virtual slot, which must be implemented for a child class.
 * Any functions that need to be called from outside the class (e.g., from the HardwareManager during a scan) should be declared as slots, so that QMetaObject::invokeMethod used to activate them if the object lives in a different thread from the HardwareManager
 * It is recommended to write wrapper functions that handle both direct calls and indirect calls through QMetaObject depending on the
 * object's thread affinity.
 * The initialize function in an implementation must call testConnection() before it returns.
 *
 * The testConnection() slot should attempt to establish communication with the physical device and perform some sort of test to make sure that the connection works and that the correct device is targeted.
 * Usually, this takes the form of an ID query of some type (e.g., *IDN?).
 * After determining if the connection is successful, the testConnection() function MUST emit the connectionResult() signal with the this pointer, a boolean indicating success, and an optional error message that can be displayed in the log along with the failure notification.
 * The testConnection() slot is also called from the communication dialog to re-test a connection that is lost.
 *
 * If at any point during operation, the program loses communication with the device, the hardwareFailure() signal should be emitted with the this pointer.
 * If the object is critical (d_isCritical), any ongoing scan will be terminated and all controls will be frozen until communication is re-established by a successful call to the testConnection() function.
 * There is an automatic check of testConnection() if this occurs during a scan, and the scan will be automatically resumed if this call is successful.
 *
 * Each hardware object has a communication protocol, which MUST be created in the implementation's constructor.
 * Several types are provided (see CommunicationProtocol for details).
 * Note that GpibInstruments MUST take a QpibController pointer in their constructors, and the hardware object must live in the same thread as the GPIB controller.
 *
 * Finally, the sleep() function may be implemented in a subclass if anything needs to be done to put the device into an inactive state.
 * For instance, the FlowController turns off gas flows to conserve sample, and the PulseGenerator turns off all pulses.
 * If sleep() is implemented, it is recommneded to explicitly call HardwareObject::sleep(), as this will display a message in the log stating that the device is in fact asleep.
 */
class HardwareObject : public QObject
{
	Q_OBJECT
public:
    /*!
     * \brief Constructor. Does nothing.
     *
     * \param parent Pointer to parent QObject. Should be 0 if it will be in its own thread.
     */
    explicit HardwareObject(QObject *parent = nullptr);
    virtual ~HardwareObject();

    /*!
     * \brief Access function for pretty name.
     * \return Name for display on UI
     */
	QString name() { return d_prettyName; }

    /*!
     * \brief Access function for key.
     * \return Name for use in the settings file
     */
	QString key() { return d_key; }

    QString subKey() { return d_subKey; }

    bool isCritical() { return d_isCritical; }

    bool isConnected() { return d_isConnected; }
    void setConnected(bool connected) { d_isConnected = connected; }

    CommunicationProtocol::CommType type() { return p_comm->type(); }
	
signals:
    /*!
     * \brief Displays a message on the log.
     * \param QString The message to display
     * \param QtFTM::LogMessageCode The status incidator (Normal, Warning, Error, Highlight)
     */
	void logMessage(const QString, const QtFTM::LogMessageCode = QtFTM::LogNormal);

    /*!
     * \brief Indicates whether a connection is successful
     * \param HardwareObject* This pointer
     * \param bool True if connection is successful
     * \param msg If unsuccessful, an optional message to display on the log tab along with the failure message.
     */
    void connected(bool success = true,QString msg = QString());

    /*!
     * \brief Signal emitted if communication to hardware is lost.
     * \param HardwareObject* This pointer
     */
    void hardwareFailure();
	
public slots:
    /*!
     * \brief Attempt to communicate with hardware. Must emit connectionResult(). Pure virtual.
     * \return Whether attempt was successful
     */
	virtual bool testConnection() =0;

    /*!
     * \brief Do any needed initialization prior to connecting to hardware. Pure virtual
     *
     */
	virtual void initialize() =0;

    /*!
     * \brief Puts device into a standby mode. Default implementation puts a message in the log.
     * \param b If true, go into standby mode. Else, active mode.
     */
	virtual void sleep(bool b);

protected:
    QString d_prettyName; /*!< Name to be displayed on UI */
    QString d_key; /*!< Name to be used in settings */
    QString d_subKey;

    CommunicationProtocol *p_comm;
    bool d_isCritical;

private:
    bool d_isConnected;
};

#endif // HARDWAREOBJECT_H
