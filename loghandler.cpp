#include "loghandler.h"
#include <QDateTime>
#include <QDate>
#include <QApplication>
#include <QSettings>


LogHandler::LogHandler(QObject *parent) :
    QObject(parent)
{
	d_currentMonth = QDate::currentDate().month();
	d_logFile.setFileName(makeLogFileName());
	d_logFile.open(QIODevice::Append);
}

LogHandler::~LogHandler()
{
	if(d_logFile.isOpen())
	    d_logFile.close();
}

void LogHandler::logMessage(const QString text, const QtFTM::LogMessageCode type)
{
	QString timeStamp = QDateTime::currentDateTime().toString();
	writeToFile(text, type, timeStamp);

#ifndef QT_NO_DEBUG
	if(type == QtFTM::LogDebug)
		return;
#endif

	QString out;
	out.append(QString("<span style=\"font-size:7pt\">%1</span> ").arg(timeStamp));

	switch(type)
	{
    case QtFTM::LogWarning:
		out.append(QString("<span style=\"font-weight:bold\">Warning: %1</span>").arg(text));
		emit iconUpdate(type);
		break;
    case QtFTM::LogError:
		out.append(QString("<span style=\"font-weight:bold;color:red\">Error: %1</span>").arg(text));
		emit iconUpdate(type);
		break;
    case QtFTM::LogHighlight:
		out.append(QString("<span style=\"font-weight:bold;color:green\">%1</span>").arg(text));
		break;
	case QtFTM::LogDebug:
		out.append(QString("<span style=\"color:blue\">Debug: %1</span>").arg(text));
		break;
    case QtFTM::LogNormal:
	default:
		out.append(text);
		break;
	}

	//emit signal containing formatted message
	emit sendLogMessage(out);
}

void LogHandler::writeToFile(const QString text, const QtFTM::LogMessageCode type, const QString timeStamp)
{
    QDate now = QDate::currentDate();
    if(now.month() != d_currentMonth)
    {
	   d_currentMonth = now.month();
	   QString newLogFile = makeLogFileName();

	   if(d_logFile.isOpen())
		  d_logFile.close();

	   d_logFile.setFileName(newLogFile);

	   d_logFile.open(QIODevice::Append);
    }

    if(d_logFile.isOpen())
    {
	    QString msg = QString("%1: ").arg(timeStamp);
	    switch (type)
	    {
	    case QtFTM::LogWarning:
		    msg.append(QString("[WARNING] "));
		    break;
	    case QtFTM::LogError:
		    msg.append(QString("[ERROR] "));
		    break;
	    case QtFTM::LogDebug:
		    msg.append(QString("[DEBUG] "));
		    break;
	    default:
		    break;
	    }

	    msg.append(text).append(QString("\n"));

	    d_logFile.write(msg.toLatin1());
	    d_logFile.flush();
    }
}


QString LogHandler::makeLogFileName()
{
    QString month;
    if(d_currentMonth < 10)
	   month = QString("0%1").arg(d_currentMonth);
    else
	   month = QString::number(d_currentMonth);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    return QString("%1/log/%2%3.log").arg(s.value(QString("savePath")).toString()).arg(QDate::currentDate().year()).arg(month);

}
