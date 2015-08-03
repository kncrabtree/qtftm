#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#include <QString>
#include <QMetaType>

namespace QtFTM {

enum LogMessageCode {
    LogNormal,
    LogWarning,
    LogError,
    LogHighlight,
	LogDebug
};

enum PulseActiveLevel {
    PulseLevelActiveLow,
    PulseLevelActiveHigh
};

enum PulseSetting {
    PulseDelay,
    PulseWidth,
    PulseEnabled,
    PulseLevel,
    PulseName
};

enum ScopeResolution {
    Res_1kHz,
    Res_2kHz,
    Res_5kHz,
    Res_10kHz
};

enum FlowSetting {
    FlowSettingEnabled,
    FlowSettingSetpoint,
    FlowSettingFlow,
    FlowSettingName
};

struct FlowChannelConfig {
    bool enabled;
    double setpoint;
    QString name;
};

struct PulseChannelConfig {
    int channel;
    QString channelName;
    bool enabled;
    double delay;
    double width;
    PulseActiveLevel level;

    PulseChannelConfig() : channel(-1), enabled(false), delay(-1.0), width(-1.0), level(PulseLevelActiveHigh) {}
};

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
	Attenuation,
    Categorize,
	DrCorrelation
};

struct BatchPlotMetaData {
	QtFTM::BatchType type; /*!< Type of batch acquisition */
	int scanNum; /*!< Scan number */
	double minXVal; /*!< Minimum frequency for this scan. */
	double maxXVal; /*!< Maximum frequency for this scan */
	bool isCal; /*!< Is this scan a calibration? */
	bool badTune;
	QString text; /*!< Text to be displayed on a plot (see Batch implementation) */
	bool drMatch; /*!< Whether DR resulted in depletion. Used in DrCorrelation */
	bool drRef; /*!< This is a reference scan for DR correlation */

	/*!
	 * \brief Constructor with default initialization
	 */
   BatchPlotMetaData() : type(QtFTM::SingleScan), scanNum(-1), minXVal(0.0), maxXVal(1.0), isCal(false), badTune(false),
	   text(QString()), drMatch(false), drRef(false) {}

	/*!
	 \brief Constructor with explicit initialization


	 \param t Type
	 \param n Scan Number
	 \param min Minimum frequency
	 \param max Maximum frequency
	 \param c Is calibration?
	 \param s Extra text
	*/
   explicit BatchPlotMetaData(QtFTM::BatchType t, int n, double min, double max, bool c, bool b = false, QString s = QString()) :
	  type(t), scanNum(n), minXVal(min), maxXVal(max), isCal(c), badTune(b), text(s) {}
	/*!
	 \brief Copy constructor

	 \param other Structure to copy
	*/
	BatchPlotMetaData(const BatchPlotMetaData &other) : type(other.type), scanNum(other.scanNum), minXVal(other.minXVal),
	  maxXVal(other.maxXVal), isCal(other.isCal), badTune(other.badTune), text(other.text), drMatch(other.drMatch), drRef(other.drRef) {}
};

}

Q_DECLARE_METATYPE(QtFTM::FlowSetting)
Q_DECLARE_METATYPE(QtFTM::PulseActiveLevel)
Q_DECLARE_METATYPE(QtFTM::LogMessageCode)
Q_DECLARE_METATYPE(QtFTM::ScopeResolution)

#endif // DATASTRUCTS_H

