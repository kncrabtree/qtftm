#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QObject>
#include <QString>

#include "datastructs.h"

class LogHandler : public QObject
{
    Q_OBJECT
public:
    explicit LogHandler(QObject *parent = nullptr);

signals:
	//sends the formatted messages to the UI
	void sendLogMessage(const QString);
	void sendStatusMessage(const QString);

public slots:
	//access functions for transmitting messages to UI
	void logMessage(const QString text, const QtFTM::LogMessageCode type=QtFTM::LogNormal);

};

#endif // LOGHANDLER_H
