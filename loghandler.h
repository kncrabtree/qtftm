#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QObject>
#include <QString>
#include <QFile>

#include "datastructs.h"

class LogHandler : public QObject
{
    Q_OBJECT
public:
    explicit LogHandler(QObject *parent = nullptr);
	~LogHandler();

signals:
	//sends the formatted messages to the UI
	void sendLogMessage(const QString);
	void sendStatusMessage(const QString);
	void iconUpdate(QtFTM::LogMessageCode);

public slots:
	//access functions for transmitting messages to UI
	void logMessage(const QString text, const QtFTM::LogMessageCode type=QtFTM::LogNormal);

private:
	QFile d_logFile;
	int d_currentMonth;

	void writeToFile(const QString text, const QtFTM::LogMessageCode type, const QString timeStamp);
	QString makeLogFileName();

};

#endif // LOGHANDLER_H
