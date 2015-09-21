#include "batch.h"
#include <QSettings>
#include <QApplication>

Batch::Batch(QList<QPair<Scan, bool> > l, AbstractFitter *ftr) :
    BatchManager(QtFTM::Batch,false,ftr), d_scanList(l), d_processScanIsCal(false)
{
	//since all scans are already in a list, we just have to calculate total shots
	d_totalShots = 0;
    for(int i=0; i<d_scanList.size(); i++)
        d_totalShots += d_scanList.at(i).first.targetShots();

}

Batch::Batch(int num) : BatchManager(QtFTM::Batch,true), d_loadingIndex(0), d_processScanIsCal(false)
{
    d_prettyName = QString("Batch");
    d_batchNum = num;

    int batchNum = num;
    int batchMillions = (int)floor((double)batchNum/1000000.0);
    int batchThousands = (int)floor((double)batchNum/1000.0);

    //get directory and file
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/batch/%1/%2").arg(batchMillions).arg(batchThousands));
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
            if(l.at(2).startsWith(QString("CAL")) || l.at(2).toInt() == 1)
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

void Batch::advanceBatch(const Scan s)
{
    Q_UNUSED(s)

    if(d_loading)
    {
        if(d_loadingIndex >= d_loadCalList.size())
            d_processScanIsCal = false;
        else
            d_processScanIsCal = d_loadCalList.at(d_loadingIndex);

        d_loadingIndex++;
    }
    else
        d_processScanIsCal = d_thisScanIsCal;
}

void Batch::processScan(Scan s)
{

    FitResult res;
    if(d_loading && d_fitter->type() == FitResult::NoFitting)
        res = FitResult(s.number());
    else
        res = d_fitter->doFit(s);

	//the scan number will be used on the X axis of the plot
	double num = (double)s.number();

    QPair<QVector<QPointF>,double> p = d_fitter->doStandardFT(s.fid());
	QVector<QPointF> ft = p.first;
	double max = p.second;

	//the data will start and end with a 0 to make the plot look a little nicer
    if(!d_processScanIsCal)
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

	//record the marker text and FT max for saving
    ScanResult sr;
    sr.scan = s;
    sr.isCal = d_processScanIsCal;
    sr.ftMax = max;
    sr.result = res;
    d_saveData.append(sr);

	//send the data to the plot
	QList<QVector<QPointF> > out;
    if(!d_processScanIsCal)
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
	QString savePath = s.value(QString("savePath"),QString(".")).toString();
	QDir d(savePath + QString("/batch/%1/%2").arg(batchMillions).arg(batchThousands));
	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
		{
			emit logMessage(QString("Could not create directory for saving batch scan! Creation of %1 failed, and data were not saved!").arg(d.absolutePath()),QtFTM::LogError);
			return;
		}
	}

	//open file for writing
	QFile out(QString("%1/%2.txt").arg(d.absolutePath()).arg(batchNum));
	QTextStream t(&out);
	QString tab = QString("\t");
	QString nl = QString("\n");
    QString zerop = QString("0.0");
    QString zero = QString("0");
    QString one = QString("1");
    QString sc = QString(";");

	if(!out.open(QIODevice::WriteOnly))
	{
		emit logMessage(QString("Could not open file for writing batch data! Creation of %1 failed, and data were not saved!").arg(out.fileName()),QtFTM::LogError);
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
    t << nl << QString("batchscan") << batchNum << tab << QString("batchmax") << batchNum << tab << QString("batchiscal") << batchNum << tab << QString("batchftfreq") << batchNum << tab;
    t << QString("batchattn") << batchNum << tab << QString("batchdrfreq") << batchNum << tab << QString("batchdrpower") << batchNum << tab << QString("batchpulses") << batchNum << tab << QString("batchshots") << batchNum << tab;
    t << QString("batchautofitpairfreqs") << batchNum << tab << QString("batchautofitpairints") << batchNum << tab << QString("batchautofitsinglefreqs") << batchNum << tab;
    t << QString("batchautofitsingleints") << batchNum << tab;
    for(int i=0;i<d_saveData.size(); i++)
    {
        const ScanResult &sr = d_saveData.at(i);
        t << nl << sr.scan.number() << tab << sr.ftMax << tab << sr.isCal << tab << sr.scan.ftFreq() << tab << sr.scan.attenuation() << tab << sr.scan.drFreq() << tab << sr.scan.drPower() << tab;

        PulseGenConfig pc = sr.scan.pulseConfiguration();
        QString pulses;
        for(int j=0; j<pc.size(); j++)
        {
            if(pc.setting(j,QtFTM::PulseEnabled).toBool())
                pulses.append(one);
            else
                pulses.append(zero);
        }
        t << pulses << tab << sr.scan.completedShots() << tab;

        if(sr.result.freqAmpPairList().isEmpty())
            t << zerop << tab << zerop;
        else
        {
            auto list = sr.result.freqAmpPairList();
            t << list.first().first;
            for(int j=1; j<list.size(); j++)
                t << sc << list.at(j).first;

            t << tab << list.first().second;
            for(int j=1; j<list.size(); j++)
                t << sc << list.at(j).second;
        }

        t << tab;
        if(sr.result.freqAmpSingleList().isEmpty())
            t << zerop << tab << zerop;
        else
        {
            auto list = sr.result.freqAmpSingleList();
            t << list.first().first;
            for(int j=1; j<list.size(); j++)
                t << sc << list.at(j).first;

            t << tab << list.first().second;
            for(int j=1; j<list.size(); j++)
                t << sc << list.at(j).second;
        }
    }

	t.flush();
	out.close();

	//increment batch number
	s.setValue(d_numKey,batchNum+1);
	s.sync();
}
