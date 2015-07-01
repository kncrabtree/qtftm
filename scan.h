#ifndef SCAN_H
#define SCAN_H

#include <QSharedDataPointer>
#include <QString>
#include <QList>
#include "pulsegenerator.h"
#include "fid.h"
#include <QStringList>
#include <QDateTime>

class ScanData;

/*!
 \brief Implicitly shared data storage wrapper for an acquisition

 This class uses an implicitly sharied ScanData pointer to provide an easy-to-use container for storing all relevant information about a single acquisition.
 Most of the functions are self-explanatory: the const access functions allow data to be read, and the non-const functions allow settings to be made, but will cause a copy-on-write if multiple references to the same object exist.
 In addition to data storage, there are functions for saving and loading scan data.

 When a scan is built from its default constructor, all data members are given invalid values.
 In particular, the Fid object is empty; this is used to test whether the scan is valid after attmepting a load operation using the numeric constructor.
 Settings are made from the user interface in some way, either from a batch operation or from explicit settings given by the user in a dialog.
 Once the Scan is sent to the HardwareManager from the AcquisitionManager, the settings are read from the Scan, and the actual settings read back from the hardware are stored to ensure that the records are accurate.
 If all initialization is successful, the initializationComplete() function is called to indicate success.
 At that point, the scan proceeds until the target number of shots is reached (isAcquisitionComplete()) or the scan is aborted (abortScan()).
 Upon completion, the save() function is called.

 When saved, the current scan number is read from persistent settings, and a data file with that number is generated and stored in /home/data/QtFtm/scans/x/y/z.txt, where x is millions, y is thousands, and z is the scan number.
 After saving, the scan number in persistent settings is incremented.
 The constructor from a scan number attempts to parse the file located at that same location.
*/
class Scan
{
public:
/*!
 \brief Default constructor.

 Sets all parameters to invalid values

*/
	Scan();
/*!
 \brief Load scan with specified number from disk

 If the file cannot be read or parsed, a default invalid scan is returned.

 \param num Scan number to load
*/
	Scan(int num);
/*!
 \brief Copy constructor

 Performs member-by-member copy of ScanData object

 \param Scan Scan to copy
*/
	Scan(const Scan &);
	/*!
	 \brief Assignment operator

	 \param Scan Scan to copy
	 \return Scan& Reference to a scan object built from the given scan
	*/
	Scan &operator=(const Scan &);
	/*!
	 \brief Equality operator

	 Tests if the two scans refer to the same ScanData object

	 \param Scan to compare
	 \return bool True if the two objects point to the same ScanData object
	*/
    bool operator ==(const Scan &other) const;
	/*!
	 \brief Destructor. Does nothing

	 Required for implicit sharing.

	*/
	~Scan();

	//access functions
	/*!
	 \brief

	 \return int
	*/
	int number() const;
	/*!
	 \brief

	 \return QDateTime
	*/
	QDateTime timeStamp() const;
	/*!
	 \brief

	 \return Fid
	*/
	Fid fid() const;
	/*!
	 \brief

	 \return double
	*/
	double ftFreq() const;
    /*!
	 \brief

	 \return double
	*/
	double drFreq() const;
	/*!
	 \brief

	 \return int
	*/
	int attenuation() const;
	/*!
	 \brief

	 \return double
	*/
	double drPower() const;
	/*!
	 \brief

	 \return double
	*/
	double pressure() const;
	/*!
	 \brief

	 \return QStringList
	*/
	QStringList gasNames() const;
	/*!
	 \brief

	 \return QList<double>
	*/
	QList<double> gasFlows() const;

    double repRate() const;
	/*!
	 \brief

	 \return QList<PulseGenerator::PulseChannelConfiguration>
	*/
	QList<PulseGenerator::PulseChannelConfiguration> pulseConfiguration() const;
	/*!
	 \brief

	 \return int
	*/
	int completedShots() const;
	/*!
	 \brief

	 \return int
	*/
	int targetShots() const;
	/*!
	 \brief

	 \return bool
	*/
	bool isInitialized() const;
	/*!
	 \brief

	 \return bool
	*/
	bool isAcquisitionComplete() const;
	/*!
	 \brief

	 \return bool
	*/
	bool isSaved() const;
	/*!
	 \brief

	 \return bool
	*/
	bool isAborted() const;
	/*!
	 \brief

	 \return bool
	*/
	bool isDummy() const;

	bool skipTune() const;

    int tuningVoltage() const;

    bool tuningVoltageTakenWithScan() const;

    int scansSinceTuningVoltageTaken() const;

    int cavityVoltage() const;

    int protectionDelayTime() const;

    int scopeDelayTime() const;

    double dipoleMoment() const;

    bool magnet() const;

	/*!
	 \brief

	 \param n
	*/
	void setNumber(int n);
	/*!
	 \brief

	*/
	void increment();
	/*!
	 \brief

	 \param f
	*/
	void setFid(const Fid f);
	/*!
	 \brief

	 \param f
	*/
	void setProbeFreq(const double f);
	/*!
	 \brief

	 \param f
	*/
	void setFtFreq(const double f);
	/*!
	 \brief

	 \param f
	*/
	void setDrFreq(const double f);
	/*!
	 \brief

	 \param a
	*/
	void setAttenuation(const int a);
	/*!
	 \brief

	 \param p
	*/
    void setCavityVoltage(const int a);
    /*!
     \brief

     \param p
    */

    void setProtectionDelayTime(const int a);

    void setScopeDelayTime(const int a);

	void setDrPower(const double p);

    void setDipoleMoment(const double a);
	/*!
	 \brief

	 \param p
	*/
	void setPressure(const double p);
	/*!
	 \brief

	 \param s
	*/
	void setGasNames(const QStringList s);
	/*!
	 \brief

	 \param f
	*/
	void setGasFlows(const QList<double> f);
	/*!
	 \brief

	 \param n
	*/
	void setTargetShots(int n);

    void setRepRate(const double rr);
	/*!
	 \brief

	 \param p
	*/
	void setPulseConfiguration(const QList<PulseGenerator::PulseChannelConfiguration> p);
	/*!
	 \brief

	*/
	void initializationComplete();
	/*!
	 \brief

	*/
	void save();
	/*!
	 \brief

	*/
	void abortScan();
	/*!
	 \brief

	*/
	void setDummy();

	void setSkiptune(bool b);

    void setTuningVoltage(int v);

    void setTuningVoltageTakenWithScan( bool b);

    void setScansSinceTuningVoltageTaken( int i);

    void setMagnet(bool b);

	/*!
	 \brief

	 \return QString
	*/
	QString scanHeader() const;
	
private:
	QSharedDataPointer<ScanData> data; /*!< Implicitly shared data storage */

	/*!
	 \brief

	 \param num
	*/
	void parseFile(int num);
	/*!
	 \brief

	 \param s
	*/
	void parseFileLine(QString s);
};

#endif // SCAN_H
