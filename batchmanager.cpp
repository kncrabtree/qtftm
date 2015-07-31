#include "batchmanager.h"
#include <QSettings>
#include <QApplication>

BatchManager::BatchManager(QtFTM::BatchType b, bool load, AbstractFitter *ftr) :
    QObject(), d_batchType(b), d_fitter(ftr), d_batchNum(-1), d_loading(load), d_thisScanIsCal(false), d_sleep(false)
{
	switch(b) {
	case QtFTM::Survey:
		d_numKey = QString("surveyNum");
		break;
	case QtFTM::DrScan:
		d_numKey = QString("drNum");
		break;
	case QtFTM::Batch:
		d_numKey = QString("batchNum");
		break;
	case QtFTM::Attenuation:
		d_numKey = QString("batchAttnNum");
		break;
	case QtFTM::DrCorrelation:
		d_numKey = QString("drCorrNum");
		break;
	default:
		d_numKey = QString("");
		break;
	}

	if(!d_loading && !d_numKey.isEmpty())
	{
		QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
		d_batchNum = s.value(d_numKey,1).toInt();
	}
}

BatchManager::~BatchManager()
{
	delete d_fitter;
}

QString BatchManager::title()
{
    if(d_batchType != QtFTM::SingleScan)
    {
        return QString("%1 %2").arg(d_prettyName).arg(d_batchNum);
    }

    return QString();
}

QPair<int, int> BatchManager::loadScanRange()
{
    QPair<int,int> out(0,0);

    if(d_loadScanList.size()>1)
    {
        out.first = d_loadScanList.at(0);
        out.second = d_loadScanList.at(d_loadScanList.size()-1);
    }

    return out;
}


void BatchManager::scanComplete(const Scan s)
{
    if(!s.isInitialized() && !s.isDummy())
	{
		//this means there was a hardware error. Abort immediately
        writeReport();
		emit batchComplete(true);
		return;
	}

    advanceBatch(s);

	if(!s.isAborted() && !isBatchComplete())
	{
		//proceed to next scan
        Scan next = prepareNextScan();
        emit beginScan(next,d_thisScanIsCal);

        //process the scan now that next scan has started
        processScan(s);
        if(!s.isDummy())
            emit processingComplete(s);
	}
	else
	{
        //only send out message if this is not a single scan
        if(d_batchType != QtFTM::SingleScan && d_batchType != QtFTM::Attenuation)
		{
            emit logMessage(QString("%1 %2 complete. Final scan: %3.").arg(d_prettyName).arg(d_batchNum).arg(s.number()),
						 QtFTM::LogHighlight);
		}

        processScan(s);
        if(!s.isDummy())
            emit processingComplete(s);
        writeReport();

        emit batchComplete(s.isAborted());
	}

}

void BatchManager::beginBatch()
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	int firstScanNum = s.value(QString("scanNum"),0).toInt()+1;

    if(d_batchType != QtFTM::SingleScan && !d_loading)
    {

	   if(d_batchType != QtFTM::Attenuation)
            emit logMessage(QString("Beginning %1 %2. First scan: %3.").arg(d_prettyName).arg(d_batchNum).arg(firstScanNum),
                            QtFTM::LogHighlight);
    }

    emit titleReady(title());

    if(!d_loading)
    {
        Scan next = prepareNextScan();
        emit beginScan(next,d_thisScanIsCal);
    }
    else
        loadBatch();
}

void BatchManager::loadBatch()
{
    if(d_batchType != QtFTM::Attenuation)
    {
        for(int i=0;i<d_loadScanList.size(); i++)
        {
            Scan s(d_loadScanList.at(i));
            if(s.number() < 1)
            {
                emit batchComplete(true);
                return;
            }
            advanceBatch(s);
            processScan(s);
            emit processingComplete(s);
        }
        //if there was nothing in the scan list there was a loading error, and the UI will display an error message
        emit batchComplete(d_loadScanList.isEmpty());
    }
    else
    {
        for(int i=0;i<d_loadAttnList.size(); i++)
            processScan(d_loadAttnList.at(i));

        emit batchComplete(d_loadAttnList.isEmpty());
    }
}


QList<QPair<QString, QtFTM::BatchType> > BatchManager::typeList()
{
    QList<QPair<QString, QtFTM::BatchType> > out;
    out.append(QPair<QString, QtFTM::BatchType>(QString("Survey"),QtFTM::Survey));
    out.append(QPair<QString, QtFTM::BatchType>(QString("DR Scan"),QtFTM::DrScan));
    out.append(QPair<QString, QtFTM::BatchType>(QString("Batch"),QtFTM::Batch));

    return out;
}
