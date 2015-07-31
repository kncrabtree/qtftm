#include "drcorrelation.h"

#include "abstractfitter.h"

DrCorrelation::DrCorrelation(QList<QPair<Scan,bool>> templateList, AbstractFitter *ftr) :
    BatchManager(QtFTM::DrCorrelation,false,ftr), d_thisScanIsRef(false), d_processScanIsCal(false),
    d_processScanIsRef(false), d_loadIndex(0)
{
	d_prettyName = QString("DR Correlation");

	for(int i=0; i<templateList.size();i++)
	{
		//for each scan in the list, we need to tune and make sure DR is off
		auto p = templateList.at(i);
		bool cal = p.second;
		Scan s = p.first;
		s.setSkiptune(false);
		PulseGenConfig pc = s.pulseConfiguration();
		pc.setDrEnabled(false);
		s.setPulseConfiguration(pc);
		d_scanList.append(qMakePair(s,cal));

		//now that the scan in the list, can modify it again safely (implicit sharing)
		//every other time this scan is used, it will be in a DR batch
		//so we skip tuning, and enable DR
		//DR frequency will be set in the loop below
		s.setSkiptune(true);
		pc.setDrEnabled(true);
		s.setPulseConfiguration(pc);

		//if the scan is a calibration, we can just go on to the next item.
		//otherwise, we need to create a DR scan for every other non-cal scan in the template list
		if(cal)
			continue;
		else
		{
			for(int j=i+1;j<templateList.size(); j++)
			{
				//ignore if this is a cal scan
				if(templateList.at(j).second)
					continue;

				//set DR frequency; reusing the scan s from above
				s.setDrFreq(templateList.at(j).first.ftFreq());
				d_scanList.append(qMakePair(s,false));
			}
		}
	}

	//scan list is now built; calculate total number of shots
	d_totalShots = 0;
	for(int i=0; i<d_scanList.size(); i++)
		d_totalShots += d_scanList.at(i).first.targetShots();
}

DrCorrelation::DrCorrelation(int num) :
    BatchManager(QtFTM::DrCorrelation,true), d_thisScanIsRef(false), d_processScanIsCal(false),
    d_processScanIsRef(false), d_loadIndex(0)
{
	d_prettyName = QString("DR Correlation");
	d_batchNum = num;

	int batchMillions = d_batchNum/1000000;
	int batchThousands = d_batchNum/1000;

	//create directory, if necessary
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QString savePath = s.value(QString("savePath"),QString(".")).toString();
	QDir d(savePath + QString("/drcorr/%1/%2").arg(batchMillions).arg(batchThousands));
	QFile in(QString("%1/%2.txt").arg(d.absolutePath()).arg(d_batchNum));

	if(in.open(QIODevice::ReadOnly))
	{
		while(!in.atEnd())
		{
			QByteArray line = in.readLine();

			if(line.isEmpty())
				continue;

			if(line.startsWith("drc") || line.startsWith("#"))
				continue;

			QByteArrayList l = line.split('\t');
			if(l.size() != 6)
				continue;

			bool ok = false;
			int scanNum = l.first().toInt(&ok);
			if(!ok)
				continue;

			bool isCal = static_cast<bool>(l.at(1).toInt(&ok));
			if(!ok)
				continue;

			bool isRef = static_cast<bool>(l.at(2).toInt(&ok));
			if(!ok)
				continue;

			d_loadScanList.append(scanNum);
			d_loadCalList.append(isCal);
			d_loadRefList.append(isRef);
		}
		in.close();
	}

}

DrCorrelation::~DrCorrelation()
{

}



void DrCorrelation::writeReport()
{
	if(d_saveData.isEmpty())
	 {
		 emit logMessage(QString("Did not create DR Correlation report because no scans were completed."),QtFTM::LogWarning);
		 return;
	 }

	//figure out where to save the data
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	int batchNum = s.value(d_numKey,1).toInt();
	int batchMillions = batchNum/1000000;
	int batchThousands = batchNum/1000;

	//create directory, if necessary
	QString savePath = s.value(QString("savePath"),QString(".")).toString();
	QDir d(savePath + QString("/drcorr/%1/%2").arg(batchMillions).arg(batchThousands));
	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
		{
			emit logMessage(QString("Could not create directory for saving DR Correlation! Creation of %1 failed, and data were not saved!").arg(d.absolutePath()),QtFTM::LogError);
			return;
		}
	}

	//open file for writing
	QFile out(QString("%1/%2.txt").arg(d.absolutePath()).arg(batchNum));
	QTextStream t(&out);
	QString tab = QString("\t");
	QString nl = QString("\n");

	if(!out.open(QIODevice::WriteOnly))
	{
		emit logMessage(QString("Could not open file for writing DR Correlation data! Creation of %1 failed, and data were not saved!").arg(out.fileName()),QtFTM::LogError);
		return;
	}

	//header stuff
	t << QString("#DR Correlation") << tab << batchNum << tab << nl;
	t << QString("#Date") << tab << QDateTime::currentDateTime().toString() << tab << nl;
    t << QString("#FID delay") << tab << d_fitter->delay() << tab << QString("us") << nl;
    t << QString("#FID high pass") << tab << d_fitter->hpf() << tab << QString("kHz") << nl;
    t << QString("#FID exp decay") << tab << d_fitter->exp() << tab << QString("us") << nl;
    t << QString("#FID remove baseline") << tab << (d_fitter->removeDC() ? QString("Yes") : QString("No")) << tab << nl;
    t << QString("#FID zero padding") << tab << (d_fitter->autoPad() ? QString("Yes") : QString("No")) << tab << nl;

	t << nl << QString("drcscan") << batchNum << tab << QString("drciscal") << batchNum << tab << QString("drcisref") << batchNum << tab
	  << QString("drcftmfreq") << batchNum << tab << QString("drcdrfreq") << batchNum << tab << QString("drcmax") << batchNum << nl;

	for(int i=0; i<d_saveData.size(); i++)
		t << nl << d_saveData.at(i);

	t.flush();
	out.close();

	s.setValue(d_numKey,batchNum+1);
    s.sync();
}

void DrCorrelation::advanceBatch(const Scan s)
{
    d_processScanIsCal = d_thisScanIsCal;
    if(!d_processScanIsCal)
        d_processScanIsRef = !s.pulseConfiguration().isDrEnabled();
    else
        d_processScanIsRef = false;
}

void DrCorrelation::processScan(Scan s)
{
	//maybe do something intelligent with this later?
	d_fitter->doFit(s);

	//the scan number will be used on the X axis of the plot
	double num = (double)s.number();

	QPair<QVector<QPointF>,double> p = d_fitter->doStandardFT(s.fid());
	QVector<QPointF> ft = p.first;
	double max = p.second;

	//the data will start and end with a 0 to make the plot look a little nicer
    if(!d_processScanIsCal)
	{
		d_drData.append(QPointF(num - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9,0.0));
		for(int i=0; i<ft.size(); i++) // make the x range go from num - 0.45 to num + 0.45
			d_drData.append(QPointF(num - ((double)ft.size()/2.0 - (double)i)/(double)ft.size()*0.9,ft.at(i).y()));
		d_drData.append(QPointF(num - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9,0.0));
	}
	else
		d_calData.append(QPointF(num,max));

	//make descriptive text to display on the plot
	QString markerText;
	QTextStream t(&markerText);
    if(d_processScanIsCal)
		t << QString("CAL\n");

	//show ftm frequency and attenuation
	t << QString("ftm:") << QString::number(s.ftFreq(),'f',1) << QString("/") << s.attenuation();

    if(!d_processScanIsCal)
	{
		//if DR is on, show DR freq and power
		if(s.pulseConfiguration().at(3).enabled)
			t  << QString("\n") << QString("dr:") << QString::number(s.drFreq(),'f',1)
			   << QString("/") << QString::number(s.drPower(),'f',1);
		t << QString("\n");
		//show pulse configuration
		for(int i=0;i<s.pulseConfiguration().size();i++)
		{
			if(s.pulseConfiguration().at(i).enabled)
				t << 1;
			else
				t << 0;
		}
	}
    if(d_processScanIsRef)
		t << QString("\n") << QString("REF");

	t.flush();

	 //make metadata
	double mdmin = static_cast<double>(s.number());
	double mdmax = static_cast<double>(s.number());
    if(!d_processScanIsCal)
	{
	    mdmin = num - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9;
	    mdmax = num - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9;
	}
	bool badTune = s.tuningVoltage() <= 0;
    QtFTM::BatchPlotMetaData md(QtFTM::Batch,s.number(),mdmin,mdmax,d_processScanIsCal,badTune,markerText);

    md.drRef = d_processScanIsRef;

	//if this scan is the reference (DR off), save the voltage. Otherwise, compare the voltage
    if(!d_processScanIsCal)
	{
		bool drMatch = false;
        if(d_processScanIsRef)
			d_currentRefMax = max;
		else
			drMatch = (max < QTFTM_DRCORR_THRESHOLD*d_currentRefMax);

		md.drMatch = drMatch;
	}
	else
		md.drMatch = false;

	 //record data for saving
	QString tab("\t");
	QString sd = QString::number(s.number()) + tab;
    if(d_processScanIsCal)
		sd.append("1");
	else
		sd.append("0");

    if(d_processScanIsRef)
		sd += tab + QString("1");
	else
		sd += tab + QString("0");

	sd += tab + QString::number(s.ftFreq(),'f',4) + tab + QString::number(s.drFreq(),'f',4) + tab + QString::number(max,'f',2);
	d_saveData.append(sd);

	//send the data to the plot
	QList<QVector<QPointF> > out;
    if(!d_processScanIsCal)
		out.append(d_drData);
	else
		out.append(d_calData);

	emit plotData(md,out);
}

Scan DrCorrelation::prepareNextScan()
{
	auto p = d_scanList.takeFirst();
	d_thisScanIsCal = p.second;
	if(!d_thisScanIsCal)
	{
		PulseGenConfig pc = p.first.pulseConfiguration();
		if(!pc.isDrEnabled())
			d_thisScanIsRef = true;
		else
			d_thisScanIsRef = false;
	}
	else
		d_thisScanIsRef = false;

	return p.first;

}

bool DrCorrelation::isBatchComplete()
{
	return d_scanList.isEmpty();
}
