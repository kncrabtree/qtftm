#ifndef PULSEGENERATOR_H
#define PULSEGENERATOR_H

#include "rs232instrument.h"

/*!
 * \brief The PulseGenerator class
 *
 * All commands sent to the 9518 must be terminated with CRLF (0x0d 0x0a).
 * The instrument responds to commands with 0x0d 0x0a, and to queries with [answer] 0x0d 0x0a, so queryCmd should be used in place of writeCmd
 */
class PulseGenerator : public Rs232Instrument
{
	Q_OBJECT
public:
    explicit PulseGenerator(QObject *parent = nullptr);

	enum ActiveLevel { ActiveHigh, ActiveLow };
	enum Setting { Delay, Width, Enabled, Active, Name };

	struct PulseChannelConfiguration {
		int channel;
		QString channelName;
		bool enabled;
		double delay;
		double width;
		ActiveLevel active;

		PulseChannelConfiguration()
			: channel(-1), channelName(QString()), enabled(false), delay(0.0), width(0.0), active(ActiveHigh) {}
		explicit PulseChannelConfiguration(int ch, QString n, bool en, double d, double w, ActiveLevel a)
			: channel(ch), channelName(n), enabled(en), delay(d), width(w), active(a) {}

	};

signals:
	void newChannelSetting(int,PulseGenerator::Setting,QVariant);
	void newChannelSettingAll(PulseGenerator::PulseChannelConfiguration);
	void newSettings(QList<PulseGenerator::PulseChannelConfiguration>);
	
public slots:
    void initialize();
	bool testConnection();
    void sleep(bool b);

	QVariant setChannelSetting(const int ch, const PulseGenerator::Setting s, const QVariant val);
	PulseGenerator::PulseChannelConfiguration setChannelAll(const int ch, const double delay, const double width, const bool enabled, const ActiveLevel a);
	PulseGenerator::PulseChannelConfiguration setChannelAll(const PulseGenerator::PulseChannelConfiguration p);
	QList<PulseGenerator::PulseChannelConfiguration> setAll(const QList<PulseGenerator::PulseChannelConfiguration> l);

	QVariant readChannelSetting(const int ch, const PulseGenerator::Setting s);
	PulseGenerator::PulseChannelConfiguration readChannelAll(const int ch);
	QList<PulseGenerator::PulseChannelConfiguration> readAll();

    //returns the current configuration so that it can be reset afterwards
    QList<PulseGenerator::PulseChannelConfiguration> configureForTuning();

    void applySettings();
    void setRepRate(double rr);

private:
	QList<PulseGenerator::PulseChannelConfiguration> pulseConfig;
    bool pGenWriteCmd(QString cmd);
	
};

#endif // PULSEGENERATOR_H
