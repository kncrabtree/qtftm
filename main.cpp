#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QDesktopServices>

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#include <signal.h>
#endif

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    //QSettings information
    QApplication::setApplicationName(QString("QtFTM"));
    QApplication::setOrganizationDomain(QString("cfa.harvard.edu"));
    QApplication::setOrganizationName(QString("CfA Spectroscopy Lab"));
    QSettings::setPath(QSettings::NativeFormat,QSettings::SystemScope,QString("/home/data"));

    //test to make sure /home/data is writable
    QDir home(QString("/home/data"));
    if(!home.exists())
    {
	    QMessageBox::critical(nullptr,QString("QtFTM Error"),QString("The directory /home/data does not exist!\n\nIn order to run QtFTM, the directory /home/data must exist and be writable by all users."));
	    return -1;
    }

    QFile testFile(QString("%1/test").arg(home.absolutePath()));
    if(!testFile.open(QIODevice::WriteOnly))
    {
	    QMessageBox::critical(nullptr,QString("QtFTM Error"),QString("Could not write to directory /home/data!\n\nIn order to run QtFTM, the directory /home/data must exist and be writable by all users."));
	    return -1;
    }
    testFile.close();
    testFile.remove();

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QDir d(s.fileName());
    d.cdUp();
    QString fileName = QString("%1/bbacq.lock").arg(d.absolutePath());

    QFile lockFile(fileName);
    qint64 pid = 0;
    bool ok = false;
    if(lockFile.exists())
    {
        QString uName = QFileInfo(lockFile).owner();        

        if(lockFile.open(QIODevice::ReadOnly))
            pid = lockFile.readLine().trimmed().toInt(&ok);

        if(!ok)
		  QMessageBox::critical(nullptr,QString("QtFTM Error"),QString("An instance of Broadband FT Acquisition is running as user %1, and it must be closed before QtFTM be started.\n\nIf you are sure no other instance is running, delete the lock file (%2) and restart.").arg(uName).arg(fileName));
        else
		  QMessageBox::critical(nullptr,QString("QtFTM Error"),QString("An instance of Broadband FT Acquisition is running under PID %1 as user %2, and it must be closed before QtFTM can be started.\n\nIf process %1 has been terminated, delete the lock file (%3) and restart.").arg(pid).arg(uName).arg(fileName));

        return -1;
    }
    else
    {
	    fileName = QString("%1/qtftm.lock").arg(d.absolutePath());
	    lockFile.setFileName(fileName);

	    if(lockFile.open(QIODevice::ReadOnly))
	    {
		    pid = lockFile.readLine().trimmed().toInt(&ok);
            QString uName = QFileInfo(lockFile).owner();

		    if(!ok)
			    QMessageBox::critical(nullptr,QString("QtFTM Error"),QString("Another instance of QtFTM is already running as user %1, and it must be closed before a new one can be started.\n\nIf you are sure no other instance is running, delete the lock file (%2) and restart.").arg(uName).arg(fileName));
		    else
			    QMessageBox::critical(nullptr,QString("QtFTM Error"),QString("Another instance of QtFTM is already running under PID %1 as user %2, and it must be closed before a new one can be started.\n\nIf process %1 has been terminated, delete the lock file (%3) and restart.").arg(pid).arg(uName).arg(fileName));

		    return -1;
	    }
	    else
	    {
		    lockFile.open(QIODevice::WriteOnly);
		    lockFile.write(QString("%1\n\nStarted by user %2 at %3.").arg(a.applicationPid()).arg(QFileInfo(lockFile).owner()).arg(QDateTime::currentDateTime().toString(Qt::ISODate)).toLatin1());
		    lockFile.close();
	    }
    }

    //all other files (besides lock file) created with this program should have 664 permissions (directories 775)
#ifdef Q_OS_UNIX
    umask(S_IWOTH);
    signal(SIGPIPE,SIG_IGN);
#endif

    //register custom types with meta object system
    qRegisterMetaType<QVector<QPointF> >("QVector<QPointF>");
    qRegisterMetaType<Fid>("Fid");
    qRegisterMetaType<QtFTM::LogMessageCode>("QtFTM::LogMessageCode");
    qRegisterMetaType<Scan>("Scan");
    qRegisterMetaType<PulseGenerator::PulseChannelConfiguration>("PulseGenerator::PulseChannelConfiguration");
    qRegisterMetaType<PulseGenerator::Setting>("PulseGenerator::Setting");
    qRegisterMetaType<QList<PulseGenerator::PulseChannelConfiguration> >("QList<PulseGenerator::PulseChannelConfiguration>");
    qRegisterMetaType<BatchManager::BatchPlotMetaData>("BatchManager::BatchPlotMetaData");
    qRegisterMetaType<QList<QVector<QPointF> > >("QList<QVector<QPointF> >");
    qRegisterMetaType<FlowConfig>("FlowConfig");
    qRegisterMetaType<FitResult>("FitResult");
    qRegisterMetaType<QtFTM::FlowSetting>("QtFTM::FlowSetting");

	MainWindow w;
	w.show();
	
    int ret = a.exec();
    lockFile.remove();
    return ret;
}
