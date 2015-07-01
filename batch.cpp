#include "batch.h"
#include <QSettings>
#include <QApplication>

Batch::Batch(QList<QPair<Scan, bool> > l, AbstractFitter *ftr) :
    BatchManager(BatchManager::Batch,false,ftr), d_scanList(l)
{
	//since all scans are already in a list, we just have to calculate total shots
	d_totalShots = 0;
    for(int i=0; i<d_scanList.size(); i++)
        d_totalShots += d_scanList.at(i).first.targetShots();

	d_numKey = QString("batchNum");
	d_prettyName = QString("Batch");

}

Batch::Batch(int num) : BatchManager(BatchManager::Batch,true), d_loadingIndex(0)
{
    d_numKey = QString("batchNum");
    d_prettyName = QString("Batch");
    d_batchNum = num;

    int batchNum = num;
    int batchMillions = (int)floor((double)batchNum/1000000.0);
    int batchThousands = (int)floor((double)batchNum/1000.0);

    //get directory and file
    QDir d(QString("/home/data/QtFTM/batch/%1/%2").arg(batchMillions).arg(batchThousands));
    QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(batchNum));

    if(!f.exists())
        return;

    if(!f.open(QIODevice::ReadOnly))
        return;

    while(!f.atEnd())
    {
        QString line = f.readLine();
        if(line.startsWith(QString("batchscan")))
            break;
    }

    while(!f.atEnd())
    {
        bool ok = false;
        QString line = f.readLine();
        QStringList l = line.split(QString("\t"));
        if(l.size()<3)
            break;

        int scanNum = l.at(0).trimmed().toInt(&ok);
        if(!ok)
            break;

        if(scanNum>0)
        {
            d_loadScanList.append(scanNum);
            if(l.at(2).startsWith(QString("CAL")))
                d_loadCalList.append(true);
            else
                d_loadCalList.append(false);
        }
    }

    f.close();


}

Scan Batch::prepareNextScan()
{
	//pop the first item off the list, and keep track of whether it's a calibration scan
    QPair<Scan,bool> p = d_scanList.takeFirst();
    d_thisScanIsCal = p.second;
    return p.first;
}

bool Batch::isBatchComplete()
{
	//the batch is complete when the list is empty
    return d_scanList.isEmpty();
}

void Batch::processScan(Scan s)
{
    if(d_loading)
    {
        if(d_loadingIndex >= d_loadCalList.size())
            d_thisScanIsCal = false;
        else
            d_thisScanIsCal = d_loadCalList.at(d_loadingIndex);

        d_loadingIndex++;
    }

    d_fitter->doFit(s);

	//the scan number will be used on the X axis of the plot
	double num = (double)s.number();

    QPair<QVector<QPointF>,double> p = d_fitter->doStandardFT(s.fid());
	QVector<QPointF> ft = p.first;
	double max = p.second;

	//the data will start and end with a 0 to make the plot look a little nicer
    if(!d_thisScanIsCal)
    {
        d_theData.append(QPointF(num - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9,0.0));
        for(int i=0; i<ft.size(); i++) // make the x range go from num - 0.45 to num + 0.45
            d_theData.append(QPointF(num - ((double)ft.size()/2.0 - (double)i)/(double)ft.size()*0.9,ft.at(i).y()));
        d_theData.append(QPointF(num - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9,0.0));
    }
    else
        d_calData.append(QPointF(num,max));

	//make descriptive text to display on the plot
	QString markerText;
	QTextStream t(&markerText);
    if(d_thisScanIsCal)
		t << QString("CAL\n");

	//show ftm frequency and attenuation
	t << QString("ftm:") << QString::number(s.ftFreq(),'f',1) << QString("/") << s.attenuation();

    if(!d_thisScanIsCal)
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
	t.flush();

	//make metadata
    double mdmin = static_cast<double>(s.number());
    double mdmax = static_cast<double>(s.number());
    if(!d_thisScanIsCal)
    {
        mdmin = num - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9;
        mdmax = num - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9;
    }
    bool badTune = s.tuningVoltage() < 0;
    BatchManager::BatchPlotMetaData md(BatchManager::Batch,s.number(),mdmin,mdmax,d_thisScanIsCal,badTune,markerText);

	//record the marker text and FT max for saving
	QPair<double,QString> scanInfo(max,markerText.replace(QString("\n"),QString("; ")));
    d_saveData.append(QPair<int,QPair<double,QString> >(s.number(),scanInfo));

	//send the data to the plot
	QList<QVector<QPointF> > out;
    if(!d_thisScanIsCal)
        out.append(d_theData);
    else
        out.append(d_calData);
	emit plotData(md,out);

}

void Batch::writeReport()
{
    if(d_saveData.isEmpty())
	{
		emit logMessage(QString("Did not create batch scan report because no scans were completed."),QtFTM::LogWarning);
		return;
	}

	//figure out where to save the data
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	int batchNum = s.value(d_numKey,1).toInt();
	int batchMillions = (int)floor((double)batchNum/1000000.0);
	int batchThousands = (int)floor((double)batchNum/1000.0);

	//create directory, if necessary
	QDir d(QString("/home/data/QtFTM/batch/%1/%2").arg(batchMillions).arg(batchThousands));
	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
		{
			emit logMessage(QString("Could not create directory for saving batch scan! Creation of %1 failed, and data were not saved!").arg(d.absolutePath()),QtFTM::LogError);
			emit fatalSaveError();
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
		emit logMessage(QString("Could not open file for writing batch data! Creation of %1 failed, and data were not saved!").arg(out.fileName()),QtFTM::LogError);
		emit fatalSaveError();
		return;
	}

	t.setRealNumberNotation(QTextStream::FixedNotation);
	t.setRealNumberPrecision(4);
	//header stuff
	t << QString("#Batch scan") << tab << batchNum << tab << nl;
	t << QString("#Date") << tab << QDateTime::currentDateTime().toString() << tab << nl;
    t << QString("#FID delay") << tab << d_fitter->delay() << tab << QString("us") << nl;
    t << QString("#FID high pass") << tab << d_fitter->hpf() << tab << QString("kHz") << nl;
    t << QString("#FID exp decay") << tab << d_fitter->exp() << tab << QString("us") << nl;
    t << QString("#FID remove baseline") << tab << (d_fitter->removeDC() ? QString("Yes") : QString("No")) << tab << nl;
    t << QString("#FID zero padding") << tab << (d_fitter->autoPad() ? QString("Yes") : QString("No")) << tab << nl;

	//write save data to file
	t << nl << QString("batchscan") << batchNum << tab << QString("batchmax") << batchNum << tab << QString("batchinfo") << batchNum;
    for(int i=0;i<d_saveData.size(); i++)
        t << nl << d_saveData.at(i).first << tab << d_saveData.at(i).second.first << tab << d_saveData.at(i).second.second;

	t.flush();
	out.close();

	//increment batch number
	s.setValue(d_numKey,batchNum+1);
	s.sync();
}
