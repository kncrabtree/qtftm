#include "batchdr.h"
#include <QSettings>
#include <QApplication>

BatchDR::BatchDR(Scan ftScan, double start, double stop, double step, int numScansBetween, QList<QPair<double, double> > ranges, bool doCal, AbstractFitter *f) :
    BatchManager(DrScan,false,f), d_template(ftScan), d_start(start), d_stop(stop), d_numScansBetween (0), d_integrationRanges(ranges),
    d_completedScans(0), d_hasCalibration(doCal)
{
	d_numKey = QString("drNum");
	d_prettyName = QString("DR Scan");

	//figure out scan direction
	if(d_stop < d_start)
		d_step = -step;
	else
		d_step = step;

	//calculate number of scans
	d_numScans = (int)ceil(fabs(d_start - d_stop)/step) + 1;

    d_numScansBetween = numScansBetween;

	//reserve space for data storage
	for(int i=0; i<ranges.size(); i++)
	{
		QVector<QPointF> d;
		d.reserve(d_numScans);
		d_drData.append(d);

		QVector<double> d2;
		d2.reserve(d_numScans);
		d_dr.append(d2);
	}

	//compute number of shots
	d_totalShots = d_numScans*ftScan.targetShots();

	//Calibration means measuring background between scans. So it doubles the length
	if(d_hasCalibration)
	{
		d_totalShots*=2;

		//need to reserve space for calibration data
		for(int i=0; i<ranges.size(); i++)
		{
			QVector<double> c;
			c.reserve(d_numScans);
			d_cal.append(c);
		}
	}
	else
	{
		//add an empty vector to cal scans for later use
		d_cal.append(QVector<double>());
    }
}

BatchDR::BatchDR(int num, AbstractFitter *ftr) : BatchManager(DrScan,true,ftr), d_completedScans(0)
{
    d_numKey = QString("drNum");
    d_prettyName = QString("DR Scan");
    d_batchNum = num;

    int drNum = num;
    int drMillions = (int)floor((double)drNum/1000000.0);
    int drThousands = (int)floor((double)drNum/1000.0);

    QDir d(QString("/home/data/QtFTM/dr/%1/%2").arg(drMillions).arg(drThousands));

    //create output file
    QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(drNum));

    if(!f.exists())
        return;

    if(!f.open(QIODevice::ReadOnly))
        return;

    //need to know 3 things from the header: step size, integration range(s) and whether calibration scans were taken
    while(!f.atEnd())
    {
        QString line = f.readLine();
        if(!line.startsWith(QString("#")))
            break;

        if(line.startsWith(QString("#Step"),Qt::CaseInsensitive))
        {
            QStringList l = line.split(QString("\t"));
            if(l.size() < 2)
                return;

            bool ok;
            d_step = l.at(1).trimmed().toDouble(&ok);
            if(!ok)
                return;

        }

        if(line.startsWith(QString("#Has calib"),Qt::CaseInsensitive))
        {
            QStringList l = line.split(QString("\t"));
            if(l.size() < 2)
                return;

            d_hasCalibration = (bool)l.at(1).trimmed().toInt();
        }

        if(line.startsWith(QString("#Range"),Qt::CaseInsensitive))
        {
            QString line2 = f.readLine();
            if(!line2.startsWith(QString("#Range"),Qt::CaseInsensitive))
                return;

            QStringList l = line.split(QString("\t"));
            QStringList l2 = line2.split(QString("\t"));

            if(l.size() < 2 || l2.size() < 2)
                return;

            bool ok;

            double start = l.at(1).toDouble(&ok);
            if(!ok)
                return;

            double stop = l2.at(1).toDouble(&ok);
            if(!ok)
                return;

            d_integrationRanges.append(QPair<double,double>(start,stop));
        }

    }

    //now we need to look for the scan list
    //if calibration scans were taken, the scan list will be two columns, drcalscans and drscans
    //otherwise, just a single column of drscans
    if(d_hasCalibration)
    {
        while(!f.atEnd())
        {
            QString line = f.readLine();
            if(line.startsWith(QString("drcalscans")))
                break;
        }

        while(!f.atEnd())
        {
            QString line = f.readLine();
            QStringList l = line.split(QString("\t"),QString::SkipEmptyParts);
            for(int i=0;i<l.size();i++)
            {
                bool ok;
                int num = l.at(i).trimmed().toInt(&ok);
                if(ok && num>0)
                    d_loadScanList.append(num);
            }
        }
    }
    else
    {
        while(!f.atEnd())
        {
            QString line = f.readLine();
            if(line.startsWith(QString("drscans")))
                break;
        }

        while(!f.atEnd())
        {
            QString line = f.readLine();
            bool ok;
            int num = line.trimmed().toInt(&ok);
            if(ok && num>0)
                d_loadScanList.append(num);
        }
    }

    d_numScans = d_loadScanList.size();

    //reserve space for data storage
    for(int i=0; i<d_integrationRanges.size(); i++)
    {
        QVector<QPointF> d;
        d.reserve(d_numScans);
        d_drData.append(d);

        QVector<double> d2;
        d2.reserve(d_numScans);
        d_dr.append(d2);
    }

    //Calibration means measuring background between scans. So it doubles the length
    if(d_hasCalibration)
    {
        d_thisScanIsCal = true;

        //need to reserve space for calibration data
        for(int i=0; i<d_integrationRanges.size(); i++)
        {
            QVector<double> c;
            c.reserve(d_numScans);
            d_cal.append(c);
        }
    }
    else
    {
        //add an empty vector to cal scans for later use
        d_cal.append(QVector<double>());
        d_thisScanIsCal = false;
    }

    f.close();
}

Scan BatchDR::prepareNextScan()
{
	//compute DR frequency
	double drFreq = d_start + (double)d_completedScans*d_step;
    int scansBetween;


	//use template, and set its frequency
	Scan next(d_template);
	next.setDrFreq(drFreq);
    scansBetween = d_completedScans % (d_numScansBetween+1);
    if( scansBetween == 0) {
        next.setTuningVoltageTakenWithScan(true);
        next.setScansSinceTuningVoltageTaken(0);
    } else {
        next.setTuningVoltageTakenWithScan(false);
        next.setScansSinceTuningVoltageTaken(scansBetween);
    }

	//make sure DR is on
	PulseGenConfig c = next.pulseConfiguration();
	c.setDrEnabled(true);

	//if there's a calibration, we might need to disable DR pulses
	if(d_hasCalibration)
	{
        if(d_thisScanIsCal) //make sure DR pulse is enabled!
            d_thisScanIsCal = false;
		else
		{
			//this is a calibration scan, so disable DR pulses
			c.setDrEnabled(false);
            d_thisScanIsCal = true;
		}
	}

	next.setPulseConfiguration(c);

    return next;
}

bool BatchDR::isBatchComplete()
{
	if(d_completedScans == d_numScans)
		return true;

	return false;
}

void BatchDR::processScan(Scan s)
{
	d_scanNumbers.append(s.number());

	//do the FT
	QVector<QPointF> ft = d_fitter->doStandardFT(s.fid()).first;
	double ftSpacing = ft.at(1).x() - ft.at(0).x();

	FitResult res;
	if(d_loading && d_fitter->type() == FitResult::NoFitting)
		res = FitResult(s.number());
	else
		res = d_fitter->doFit(s);

	if(d_fitter->type() != FitResult::NoFitting && res.category() != FitResult::Saturated && res.category() != FitResult::Invalid)
		ft = Analysis::removeBaseline(ft,res.baselineY0Slope().first,res.baselineY0Slope().second,s.fid().probeFreq());

	//do the integration... Trapezoid rule with interpolation for first and last points
	for(int i=0; i<d_integrationRanges.size(); i++)
	{
		double xMin = d_integrationRanges.at(i).first;
		double xMax = d_integrationRanges.at(i).second;
		double integral = 0.0;

		int firstIndex = (int)ceil((xMin-ft.at(0).x())/ftSpacing);
		int lastIndex = (int)floor((xMax-ft.at(0).x())/ftSpacing);

		//make sure these indices are within the range. otherwise, just integrate the whole FT
		if(firstIndex < 0 || firstIndex >= ft.size())
		{
			firstIndex = 0;
			xMin = ft.at(0).x();
		}

		if(lastIndex < 0 || lastIndex >= ft.size())
		{
			lastIndex = ft.size()-1;
			xMax = ft.at(ft.size()-1).x();
		}

		//most likely, the integration range doesn't exactly line up with x points, so we need to interpolate the ends
		if(firstIndex > 0)
		{
			//the trapezoid is defined by 4 points: (xMin,0), (xMin,y1), (x2,0), and (x2,y2)
			//x2 > xMin, y1>0, and y2>0, but we don't know if y2>y1 yet
			double x2 = ft.at(firstIndex).x();
			double dx = x2 - xMin;
			double y2 = ft.at(firstIndex).y();

			//interpolate to get y1: slope is calculated from previous point
			double y1 = y2 - (y2-ft.at(firstIndex-1).y())/ftSpacing*dx;

			//compute area; add to integral
			integral += (y2 - 0.5*(y2-y1))*dx;
		}

		if(lastIndex < ft.size()-2)
		{
			//the trapezoid is defined by 4 points: (xMax,0), (xMax,y1), (x2,0), and (x2,y2)
			//xMax > x2, y1>0, and y2>0, but we don't know if y2>y1 yet
			double x2 = ft.at(lastIndex).x();
			double dx = xMax - x2;
			double y2 = ft.at(lastIndex).y();

			//interpolate to get y1: slope is calculated from next point
			double y1 = y2 + (ft.at(lastIndex+1).y()-y2)/ftSpacing*dx;

			//compute area; add to integral
			integral += (y2 + 0.5*(y2-y1))*dx;
		}

		//now, add up chunks in between. this is easier
		for(int j=firstIndex; j< lastIndex; j++)
		{
			double y1 = ft.at(j).y();
			double y2 = ft.at(j+1).y();
			double dx = ftSpacing;

			integral += (y2 - 0.5*(y2-y1))*dx;
		}

		//record integral. store it in the appropriate vector for later processing
        if(d_thisScanIsCal)
			d_cal[i].append(integral);
		else
			d_dr[i].append(integral);
	}

	//on the plot, the data will be represented by a single point. set the range to cover halfway between adjacent points
	double plotStart = s.drFreq() - d_step/2.0, plotEnd = s.drFreq() + d_step/2.0;
	if(d_step<0.0)
		qSwap(plotStart,plotEnd);

	//only send out data if this was a real DR scan, not a calibration
    if(!d_thisScanIsCal)
	{
		//if we are doing calibration, we need to ratio the integrals after collecting a cal and a dr
		if(d_hasCalibration)
		{
			//loop over
			for(int i=0; i<d_dr.size(); i++)
			{
				//make sure cal is not 0
                if(d_cal.at(i).at(d_completedScans) > 0.0)
				{
					//compute ratio
                    double ratio = d_dr.at(i).at(d_completedScans)/d_cal.at(i).at(d_completedScans);
                    d_drData[i].append(QPointF(s.drFreq(),ratio));
				}
				else //this shouldn't happen, but append a 0
					d_drData[i].append(QPointF(s.drFreq(),0.0));
			}
            bool badTune = s.tuningVoltage() < 0;

			//send out cal and actual data together
			BatchPlotMetaData md1(DrScan,s.number()-1,plotStart,plotEnd,true);
            BatchPlotMetaData md2(DrScan,s.number(),plotStart,plotEnd,false,badTune);
			emit plotData(md1,d_drData);
			emit plotData(md2,d_drData);
		}
		else //no calibration at all, just send out the data
		{
			for(int i=0; i<d_dr.size(); i++)
                d_drData[i].append(QPointF(s.drFreq(),d_dr.at(i).at(d_completedScans)));

			BatchPlotMetaData md(DrScan,s.number(),plotStart,plotEnd,false);
			emit plotData(md,d_drData);
		}
	}

	//only increment scan if this was a DR scan, not a calibration scan
	//note that lastScanWasCal refers to the scan that is now being processed!
	//lastScanWasCal is always false for a Dr scan without calibration
    if(!d_thisScanIsCal)
		d_completedScans++;

    if(d_loading && d_hasCalibration)
        d_thisScanIsCal = !d_thisScanIsCal;
}


void BatchDR::writeReport()
{
	if(d_drData.at(0).isEmpty())
	{
		emit logMessage(QString("Did not create DR scan report because no DR scans were completed."),QtFTM::LogWarning);
		return;
	}

	//figure out where to save the data
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	int drNum = s.value(d_numKey,1).toInt();
	int drMillions = (int)floor((double)drNum/1000000.0);
	int drThousands = (int)floor((double)drNum/1000.0);

	QDir d(QString("/home/data/QtFTM/dr/%1/%2").arg(drMillions).arg(drThousands));

	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
		{
			emit logMessage(QString("Could not create directory for saving DR scan! Creation of %1 failed, and data were not saved!").arg(d.absolutePath()),QtFTM::LogError);
			return;
		}
	}

	//create output file
	QFile out(QString("%1/%2.txt").arg(d.absolutePath()).arg(drNum));
	QTextStream t(&out);
	QString tab = QString("\t");
	QString nl = QString("\n");

	if(!out.open(QIODevice::WriteOnly))
	{
		emit logMessage(QString("Could not open file for writing DR data! Creation of %1 failed, and data were not saved!").arg(out.fileName()),QtFTM::LogError);
		return;
	}

	t.setRealNumberNotation(QTextStream::FixedNotation);
	t.setRealNumberPrecision(4);
	//header stuff
	t << QString("#DR scan") << tab << drNum << tab << nl;
	t << QString("#Date") << tab << QDateTime::currentDateTime().toString() << tab << nl;
	t << QString("#Start freq") << tab << d_drData.at(0).at(0).x() << QString("\tMHz\n");
	t << QString("#End freq") << tab << d_drData.at(0).at(d_drData.at(0).size()-1).x() << QString("\tMHz\n");
	t << QString("#Step size") << tab << d_step << QString("\tMHz\n");
    t << QString("#Scans between each reading of tuning voltage")<< tab << d_numScansBetween << QString("\t\n");
	t << QString("#FT freq") << tab << d_template.ftFreq() << QString("\tMHz\n");
	t << QString("#Has calibration") << tab << d_hasCalibration << tab << nl;
	t << QString("#DR scans") << tab << d_dr.at(0).size() << tab << nl;
	t << QString("#Cal scans") << tab << d_cal.at(0).size() << tab << nl;
	t << QString("#Num ranges") << tab << d_integrationRanges.size() << tab << nl;
	for(int i=0; i<d_integrationRanges.size(); i++)
	{
		t << QString("#Range %1 start").arg(i) << tab << QString::number(d_integrationRanges.at(i).first,'f',4)
		  << tab << QString("MHz") << nl;
		t << QString("#Range %1 stop").arg(i) << tab << QString::number(d_integrationRanges.at(i).second,'f',4)
		  << tab << QString("MHz") << nl;
	}
	t << QString("#FID delay") << tab << d_fitter->delay() << tab << QString("us") << nl;
	t << QString("#FID high pass") << tab << d_fitter->hpf() << tab << QString("kHz") << nl;
	t << QString("#FID exp decay") << tab << d_fitter->exp() << tab << QString("us") << nl;
	t << QString("#FID remove baseline") << tab << (d_fitter->removeDC() ? QString("Yes") : QString("No")) << tab << nl;
	t << QString("#FID zero padding") << tab << (d_fitter->autoPad() ? QString("Yes") : QString("No")) << tab << nl;
	t << QString("#Autofit Enabled") << tab << (d_fitter->type() == FitResult::NoFitting ? QString("No") : QString("Yes")) << tab << nl;

	t << nl;
	t.setRealNumberNotation(QTextStream::ScientificNotation);
	t.setRealNumberPrecision(6);

	//make header line
	t << QString("drfreq") << drNum;
	for(int i=0; i<d_integrationRanges.size(); i++)
	{
		t << tab << QString("dr") << i << QString("int") << drNum;
		if(d_hasCalibration)
		{
			//we have cal data, so we want to add the raw and cal data
			t << tab << QString("dr") << i << QString("rawint") << drNum;
			t << tab << QString("dr") << i << QString("calint") << drNum;
		}
	}

	for(int i=0; i<d_drData.at(0).size(); i++) // loop over first data vector to get row # (i)
	{
		//record frequency
		t << nl << QString::number(d_drData.at(0).at(i).x(),'f',4);

		//loop over list of ranges, and grab the y data of each point (column # is j)
		for(int j=0; j<d_drData.size(); j++)
		{
			//this should never be an issue, but make sure we don't run over an index that is not explicitly checked
			//all data vectors are created at the same time, so they should always be the same length
			//nevertheless, don't want to cause a crash if something goes wrong...
			if(i<d_drData.at(j).size())
				t << tab << d_drData.at(j).at(i).y();
			else
				t << tab << 0.0;

			if(d_hasCalibration)
			{
				//then the raw integral
				if(i<d_dr.at(j).size())
					t << tab << d_dr.at(j).at(i);
				else
					t << tab << 0.0;

				//then the cal integral
				if(i<d_cal.at(j).size())
					t << tab << d_cal.at(j).at(i);
				else
					t << tab << 0.0;
			}
		}

	}

	//make list of scans
	t << nl << nl;
	if(d_hasCalibration)
		t << QString("drcalscans") << drNum << tab;

	t << QString("drscans") << drNum;

	for(int i=0; i<d_scanNumbers.size(); i++)
	{
		if(d_hasCalibration)
		{
			if(i%2==0)
				t << nl << d_scanNumbers.at(i);
			else
				t << tab << d_scanNumbers.at(i);
		}
		else
			t << nl << d_scanNumbers.at(i);
	}

	t.flush();
	out.close();

	//increment DR scan number
	s.setValue(d_numKey,drNum+1);
	s.sync();

}
