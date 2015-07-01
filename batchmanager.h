#ifndef BATCHMANAGER_H
#define BATCHMANAGER_H

#include <QObject>
#include <QFile>
#include <QDir>
#include "datastructs.h"
#include "scan.h"
#include "nofitter.h"
#include <QTextStream>

/*!
 \brief Abstract base class for batch acquisitions

 This class implements the common functionality for all acquisition sessions.
 Every acquisition is based on this class, even a single scan.
 Each BatchManager instance is moved into its own thread of execution, so long operations can and should block until complete.
 The BatchManager uses signals and slots to communicate with the user interface thread and the acquisition thread.
 The acquisition will begin when the beginBatch slot is triggered from the UI after all initialization and signal-slot connections are complete.
 Once going, subsequent scans are initiated by emitting the startScan signal, and processing is initiated in the scanComplete slot.
 This is all implemented in this base class; derived classes do not need to worry about these things.

 Derived classes must implement the four virtual functions: prepareScan, isBatchComplete, processScan, and writeReport.
 These are the core functions that will be called as the batch acquisition proceeds.
 Constructors of the derived classes must generate all the information necessary to allow the prepareScan function to compute the next scan, and must also set the values of d_totalShots, d_numKey, and d_prettyName.
 For example, the BatchSurvey constructor uses the start, stop, and step frequencies, along with the frequency of calibrations scans, to produce scan objects from templates.
 The prepareScan function should create the next scan object in the series and return it.
 In processScan, any analysis should be performed, and the results sent to the batch plot via the plotData signal.
 The acquisition finishes when isBatchComplete() returns true or a scan is aborted, and the writeReport function is then called.
 This function should usually generate a report in its own folder.

 Data sent to the UI is in the form of a list of vectors of points.
 For most acquisitions, the list will only have a single vector with XY data.
 However, BatchDR scans with multiple integration ranges have more than one vector of data.
 Subclasses can store their data internally in any format, but for display on the batch plot, it must be packed into a list of vectors, and the order must be the same every time.

 Batches also have a concept of calibration scans.
 Calibration scans take different forms depending on the type of batch acquisition.
 In a BatchSurvey or Batch, a calibration scan usually covers a particular line at one distinct frequency, and is used for observing changes in a reference signal over time.
 In a BatchDR scan, a calibration measures the integrated intensity in each region with the DR pulse off, to calibrate the background.
 The BatchPlot handles calibration data differently than regular scans.
*/
class BatchManager : public QObject
{
	Q_OBJECT
public:

	/*!
	 \brief Type of batch acquisition

	 Each type of batch acquisition needs to have an entry here.
	 This is used in various places when the program needs to behave differently for each type (e.g., in the BatchPlot class).
	 Note that a SingleScan is just a special version of a batch that only does one scan, and does not generate an output file.
	*/
	enum BatchType {
		SingleScan,
		Survey,
		DrScan,
        Batch,
        Attenuation
	};

     static QList<QPair<QString,BatchManager::BatchType> > typeList();

	/*!
	 \brief Structure with metadata about each scan in the batch

	 This structure is used extensively in the BatchPlot class to provide convenience functionality in the batch plot.
	*/
	struct BatchPlotMetaData {
		BatchManager::BatchType type; /*!< Type of batch acquisition */
		int scanNum; /*!< Scan number */
		double minXVal; /*!< Minimum frequency for this scan. */
		double maxXVal; /*!< Maximum frequency for this scan */
		bool isCal; /*!< Is this scan a calibration? */
        bool badTune;
		QString text; /*!< Text to be displayed on a plot (see Batch implementation) */

		/*!
		 * \brief Constructor with default initialization
		 */
        BatchPlotMetaData() : type(BatchManager::SingleScan), scanNum(-1), minXVal(0.0), maxXVal(1.0), isCal(false), badTune(false), text(QString()) {}

		/*!
		 \brief Constructor with explicit initialization


		 \param t Type
		 \param n Scan Number
		 \param min Minimum frequency
		 \param max Maximum frequency
		 \param c Is calibration?
		 \param s Extra text
		*/
        explicit BatchPlotMetaData(BatchManager::BatchType t, int n, double min, double max, bool c, bool b = false, QString s = QString()) :
            type(t), scanNum(n), minXVal(min), maxXVal(max), isCal(c), badTune(b), text(s) {}
		/*!
		 \brief Copy constructor

		 \param other Structure to copy
		*/
		BatchPlotMetaData(const BatchPlotMetaData &other) : type(other.type), scanNum(other.scanNum), minXVal(other.minXVal),
            maxXVal(other.maxXVal), isCal(other.isCal), badTune(other.badTune), text(other.text) {}
	};

	/*!
	 \brief Constructor. Initializes batch type

	 \param b Batch type
	*/
	explicit BatchManager(BatchType b, bool load = false, AbstractFitter *ftr = new NoFitter());
	~BatchManager();

	/*!
	 \brief Access function for type of batch acquisition

	 \return BatchType Batch type
	*/
	BatchType type() const { return d_batchType; }

	/*!
	 \brief Access function for number of shots

	 \return int total shots
	*/
	int totalShots() const { return d_totalShots; }

	/*!
	 \brief Sets initial truncation (\sa FTWorker)

	 \param d Delay (us)
	*/
    void setFtDelay(double d) { d_fitter->setDelay(d); }
	/*!
	 \brief Sets high pass filter cutoff frequency (\sa FTWorker)

	 \param d High pass filter cutoff (kHz)
	*/
    void setFtHpf(double d) { d_fitter->setHpf(d); }
	/*!
	 \brief Sets exponential decay filter (\sa FTWorker)

	 \param d Exponential time constant (us)
	*/
    void setFtExp(double d) { d_fitter->setExp(d); }

    void setRemoveDC(bool b) { d_fitter->setRemoveDC(b); }
    void setPadFid(bool b) { d_fitter->setAutoPad(b); }

    QString title();

    bool isLoading() const { return d_loading; }

    QPair<int,int> loadScanRange();
	
signals:
	/*!
	 \brief Sends messages to be displayed on log tab

	 \param QString Message
	 \param QtFTM::LogMessageCode Type of message
	*/
	void logMessage(const QString, const QtFTM::LogMessageCode);
	/*!
	 \brief Sends scan to acquisition manager

	 \param Scan The scan to acquire
	*/
    void beginScan(Scan, bool isCal = false);
	/*!
	 \brief Tells UI that the batch is complete

	 \param aborted True if the scan was aborted, either by user or by a hardware failure
	*/
	void batchComplete(const bool aborted = false);
	/*!
	 \brief Sends metadata and data to be plotted \sa BatchPlot

	 \param BatchManager::BatchPlotMetaData Metadata
	 \param QList<QVector<QPointF> > Data
	*/
	void plotData(const BatchManager::BatchPlotMetaData, const QList<QVector<QPointF> >);
	/*!
	 \brief Emitted in case of save failure

	*/
	void fatalSaveError();

    void titleReady(QString);

    void processingComplete(Scan s);
	
public slots:
	/*!
	 \brief Initiates processing of completed scan

	 First, this checks to see if the scan was aborted, and if so, writes a report and emits the batchComplete signal.
	 Otherwise, the scan is processed, and isBatchComplete is called to see if the scan is done.
	 If not, then the prepareScan function is called and another startScan signal emitted.

	 \param s
	*/
	void scanComplete(const Scan s);
	/*!
	 \brief Initiates acquisition by calling prepareScan and emitting startScan

	*/
	void beginBatch();

protected:
	BatchType d_batchType; /*!< Type of acquisition */

	QString d_numKey; /*!< Key in settings file for the batch count. Used to display batch number on log upon completion. Not needed for SingleScan */

	QString d_prettyName; /*!< Type of acquisition in pretty printable text. Not needed for SingleScan */

	int d_totalShots; /*!< Total number of shots to be collected for acquisition. Used for UI batch progress bar. */

	/*!
	 \brief Generates report for batch.

	 This function should generate a report (except SingleScan) in /home/data/QtFTM/x/y/z, where x is a string unique for the type (presently, survey, dr, and batch), x is the millions digit, and y is the thousands digit.
	 The file name should be num.txt, and the value of num comes from the settings key stored in d_numKey.
	 This function should increment that number in the settings when the save is successfully completed.
	*/
	virtual void writeReport() =0;
	/*!
	 \brief Process data from scan

	 In this function, all analysis for the scan is performed.
	 Generally, this will involve taking the FT of the FID, and may also involve integration or other tasks.
	 Data may be stored in any internal format, and anything to be plotted on the batch plot is sent with the plotData signal.

	 \param s
	*/
	virtual void processScan(Scan s) =0;
	/*!
	 \brief Generates next scan in the batch

	 \return Scan The next scan
	*/
	virtual Scan prepareNextScan() =0;

	/*!
	 \brief Returns whether the batch is complete

	 \return bool Is the batch complete
	*/
	virtual bool isBatchComplete() =0;

    AbstractFitter *d_fitter; /*!< Worker for computing FTs */

    int d_batchNum;
    QList<int> d_loadScanList;
    QList<Scan> d_loadAttnList;
    bool d_loading;
    bool d_thisScanIsCal;

private:
    void loadBatch();

};

#endif // BATCHMANAGER_H
