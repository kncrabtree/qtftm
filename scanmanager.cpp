#include "scanmanager.h"
#include <QSettings>
#include <QApplication>

ScanManager::ScanManager(QObject *parent) :
    QObject(parent), d_paused(false), d_acquiring(false), d_waitingForInitialization(false),
    d_connectAcqAverageAfterNextFid(false)
{

	connect(this,&ScanManager::newFid,this,&ScanManager::peakUpAverage);
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

	//these are settings that are relevant for the peak up plot
	//currentProbeFreq will be updated when the ftm synth initializes, then anytime its value changes
	d_peakUpAvgs = s.value(QString("peakUpAvgs"),20).toInt();
	d_peakUpCount = 0;
	d_currentProbeFreq = 4999.6;
}


void ScanManager::fidReceived(const QByteArray d)
{
	Fid f;

	//if a scan is active, take the probe frequency from the scan. otherwise, use most recent value
	if(d_currentScan.isInitialized() && !d_currentScan.isAcquisitionComplete())
		f = Oscilloscope::parseWaveform(d,d_currentScan.fid().probeFreq());
	else
		f = Oscilloscope::parseWaveform(d,d_currentProbeFreq);

    if(f.probeFreq()<0.0) //parsing error!
        return;

	emit newFid(f);

    if(d_connectAcqAverageAfterNextFid)
    {
        d_connectAcqAverageAfterNextFid = false;
	   connect(this,&ScanManager::newFid,this,&ScanManager::acqAverage,Qt::UniqueConnection);
    }
}

void ScanManager::prepareScan(Scan s)
{

    if(d_waitingForInitialization)
	{
		//this would be really bad... let's just hope it never happens.
		//we'll just ignore this scan, Perhaps there's a better idea?
		emit logMessage(QString("A new scan was started while hardware is being initialized for another scan. The new scan is being ignored."),LogHandler::Warning);
		return;
	}

    d_scanCopyForRetry = s;

	//Start hardware initialization
	d_waitingForInitialization = true;
    d_acquiring = true;
	emit initializeHardwareForScan(s);

}

void ScanManager::startScan(Scan s)
{
	//a dummy scan is used occasionally to configure the instrument for a scan,
	//but not to record any data.
	//for instance, when setting up a DR scan, a dummy scan is used to configure the hardware
	//and show a plot that lets the user see the signal, adjust filtering settings, and set integration ranges
	if(s.isDummy())
	{
		d_waitingForInitialization = false;
		resetPeakUpAvgs();
        emit dummyComplete(s);
		return;
	}

	if(!d_waitingForInitialization)
	{
		//scan has been aborted during hardware initialization!
		s.abortScan();
		emit acquisitionComplete(s);
		return;
	}
	d_waitingForInitialization = false;
	d_currentScan = s;

	//the Hardware manager will set the scan to initialized if it was successful
	if(d_currentScan.isInitialized())
	{
		emit initializationComplete();
        d_connectAcqAverageAfterNextFid = true;
	}
	else
	{
        d_acquiring = false;
		d_currentScan.abortScan();
		emit acquisitionComplete(d_currentScan);
	}
}

void ScanManager::peakUpAverage(const Fid f)
{
	//if we've changed frequency or oscilloscope parameters, start a new rolling average
	if(f.size() != d_peakUpFid.size() || f.probeFreq() != d_peakUpFid.probeFreq() || f.spacing() != d_peakUpFid.spacing())
		d_peakUpCount = 0;

	//if we're at the goal, set the count to 1 below
	if(d_peakUpCount >= d_peakUpAvgs)
		d_peakUpCount = d_peakUpAvgs-1;

	//if the count is 0, just use the Fid that we got
	if(d_peakUpCount == 0)
		d_peakUpFid = f;
	else
	{
		//average in the new Fid
		QVector<double> newData;
		newData.reserve(f.size());

		for(int i=0; i<f.size(); i++)
			newData.append((d_peakUpFid.at(i)*(double)d_peakUpCount+f.at(i))/((double)d_peakUpCount+1));

		d_peakUpFid = Fid(f.spacing(),f.probeFreq(),newData);
	}

	d_peakUpCount++;
	emit peakUpFid(d_peakUpFid);
}

void ScanManager::setPeakUpAvgs(int a)
{
	if(a>0)
	{
		//set the value, and record it in settings
		QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

		d_peakUpAvgs = a;
		s.setValue(QString("peakUpAvgs"),a);
		s.sync();
	}
}

void ScanManager::acqAverage(const Fid f)
{
	//pausing amounts to ignoring new FIDs that come in
	if(d_paused)
		return;

	//increment the scan, and pass along messages to UI
	d_currentScan.increment();
	int n = d_currentScan.completedShots();
	emit scanShotAcquired();
	emit statusMessage(QString("Acquiring... (%1/%2)").arg(n).arg(d_currentScan.targetShots()));

	//if the scan is complete, stop taking in new FIDs
    if(d_currentScan.isAcquisitionComplete())
    {
        d_acquiring = false;
		disconnect(this,&ScanManager::newFid,this,&ScanManager::acqAverage);
	}

	//because Fid is implicitly shared, modifying it would cause a copy on write since currentScan.fid() was sent to UI
	//it's more efficient to create a new Fid with the new data and assign it to currentScan, so that the
	//vector containing the FID data doesn't get looped over twice
	if(n>1)
	{
		//make data vector of the appropriate size
		QVector<double> newData(f.size());

		//do the rolling average, and set the FID of the scan appropriately
		for(int i=0; i<f.size(); i++)
			newData[i] = (d_currentScan.fid().at(i)*((double)n-1.0)+f.at(i))/(double)n;

		d_currentScan.setFid(Fid(d_currentScan.fid().spacing(),d_currentScan.fid().probeFreq(),newData));
	}
	else
		d_currentScan.setFid(f);

	emit scanFid(d_currentScan.fid());

	//if we're done, save and notify the rest of the program that the scan is done
	if(d_currentScan.isAcquisitionComplete())
	{
		d_currentScan.save();	
		if(!d_currentScan.isSaved())
		{
			emit logMessage(QString("Could not open file for saving scan %1").arg(d_currentScan.number()),LogHandler::Error);
			emit fatalSaveError();
        }
		emit acquisitionComplete(d_currentScan);
	}

}

void ScanManager::abortScan()
{
	//in case this happens during hardware initialization, set this flag so that the scan is ignored when it comes back
	d_waitingForInitialization = false;
	d_paused = false;

	//only do the following if the scan has been initialized, but not yet saved
	if(d_currentScan.isInitialized() && !d_currentScan.isSaved())
	{
		disconnect(this,&ScanManager::newFid,this,&ScanManager::acqAverage);
		d_currentScan.abortScan();
		d_currentScan.save();
		if(!d_currentScan.isSaved())
		{
			emit logMessage(QString("Could not open file for saving scan %1").arg(d_currentScan.number()),LogHandler::Error);
			emit fatalSaveError();
		}
        d_acquiring = false;
		emit acquisitionComplete(d_currentScan);
    }
}

void ScanManager::failure()
{
    if(d_currentScan.isInitialized() && !d_currentScan.isSaved())
        abortScan();
}

void ScanManager::retryScan()
{
    if(d_acquiring)
    {
        d_waitingForInitialization = false;
        d_paused = false;
        disconnect(this,&ScanManager::newFid,this,&ScanManager::acqAverage);

        emit logMessage(QString("Automatic error recovery invoked, retrying curent scan."),LogHandler::Highlight);
        prepareScan(d_scanCopyForRetry);
    }
}
