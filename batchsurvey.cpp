#include "batchsurvey.h"
#include <math.h>
#include <QSettings>
#include <QApplication>

BatchSurvey::BatchSurvey(Scan first, double step, double end, bool hascal, Scan cal, int scansPerCal, AbstractFitter *af) :
    BatchManager(Survey,false,af), d_surveyTemplate(first), d_hasCalibration(hascal), d_calTemplate(cal),
    d_scansPerCal(scansPerCal), d_currentSurveyIndex(0)
{
	d_numKey = QString("surveyNum");
	d_prettyName = QString("Survey");

	//10 kHz is smallest allowed step size for a survey!
	if(step < 0.01)
		step = 0.01;

	//figure out direction
	if(end < first.ftFreq())
		d_step = -step;
	else
		d_step = step;

	//calculate frequency chunk for each scan that will be displayed
	//center each chunk at the cavity frequency
	//calculate offset between probe freq and cavity freq
	//if the step size is more tham twice that, there will be gaps!
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	d_offset = s.value(QString("ftSynth/offset"),0.400).toDouble();
	d_chunkStart = qMax(d_offset - step/2.0, 0.0);
	d_chunkEnd = qMin(d_offset + step/2.0, 2.0*d_offset);


	//compute number of survey elements
    d_totalSurveyScans = (int)ceil( (end-first.ftFreq())/d_step ) + 1;

	//if there's a calibration line, it will be the first scan and the last scan
	if(d_hasCalibration)
	{
		if(d_scansPerCal > 0)
            d_totalCalScans = (int)ceil( (double)d_totalSurveyScans/(double)scansPerCal ) + 1;
		else
            d_totalCalScans = 2;

        d_thisScanIsCal = true;
	}
	else
	{
        d_totalCalScans = 0;
	}

    d_totalShots = d_totalSurveyScans*d_surveyTemplate.targetShots() + d_totalCalScans*d_calTemplate.targetShots();

}

BatchSurvey::BatchSurvey(int num) : BatchManager(Survey,true)
{
    d_numKey = QString("surveyNum");
    d_prettyName = QString("Survey");
    d_batchNum = num;
    d_thisScanIsCal = false;

    int surveyNum = num;
    int surveyMillions = (int)floor((double)surveyNum/1000000.0);
    int surveyThousands = (int)floor((double)surveyNum/1000.0);
    QDir d(QString("/home/data/QtFTM/surveys/%1/%2").arg(surveyMillions).arg(surveyThousands));

    //open file for writing
    QFile out(QString("%1/%2.txt").arg(d.absolutePath()).arg(surveyNum));

    if(!out.exists())
        return;

    if(!out.open(QIODevice::ReadOnly))
        return;

    //parse header
    bool done = false;
    while(!done && !out.atEnd())
    {
        QString line = out.readLine();
        if(!line.startsWith(QChar('#')))
        {
            done = true;
            break;
        }

        QStringList l = line.split(QString("\t"));
        if(l.size()<2)
            return;

        QString key = l.at(0);
        QString value = l.at(1);

        bool ok = true;
        if(key.startsWith(QString("#Step size"),Qt::CaseInsensitive))
            d_step = value.trimmed().toDouble(&ok);
        if(key.startsWith(QString("#Cal scans"),Qt::CaseInsensitive))
            d_hasCalibration = value.trimmed().toInt(&ok) > 0;
        if(key.startsWith(QString("#Scans per cal"),Qt::CaseInsensitive))
            d_scansPerCal = value.trimmed().toInt(&ok);

        if(!ok)
            return;

    }

    //find survey scans list
    while(!out.atEnd())
    {
        QString line = out.readLine();
        if(line.startsWith(QString("surveyscans")))
            break;
    }

    //put scan numbers into list
    while(!out.atEnd())
    {
        QString line = out.readLine();
        if(line.startsWith(QString("\n")))
            break;

        bool ok = true;
        int n = line.trimmed().toInt(&ok);

        if(ok && n>0)
            d_loadScanList.append(n);
    }

    if(d_loadScanList.isEmpty())
        return;

    d_surveyTemplate = Scan(d_loadScanList.at(0));

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    d_offset = s.value(QString("ftSynth/offset"),0.400).toDouble();
    d_chunkStart = qMax(d_offset - fabs(d_step)/2.0, 0.0);
    d_chunkEnd = qMin(d_offset + fabs(d_step)/2.0, 2.0*d_offset);
    d_currentSurveyIndex = 0;
    d_totalSurveyScans = d_loadScanList.size();
    d_totalCalScans = 0;

    if(d_hasCalibration)
    {
        d_thisScanIsCal = true;

        //find cal scans list
        while(!out.atEnd())
        {
            QString line = out.readLine();
            if(line.startsWith(QString("surveycalscans")))
                break;
        }

        //load cal scans into list
        while(!out.atEnd())
        {
            QString line = out.readLine();
            if(line.startsWith(QString("\n")))
                break;

            bool ok = true;
            int n = line.split(QString("\t")).at(0).trimmed().toInt(&ok);

            if(ok && n>0)
                d_loadScanList.append(n);
        }

        //sort list
        qSort(d_loadScanList);

        d_calTemplate = Scan(d_loadScanList.at(0));
        d_totalCalScans = d_loadScanList.size()-d_totalSurveyScans;
    }

    out.close();

}

Scan BatchSurvey::prepareNextScan()
{
	//if the next scan should be a calibration scan, use the cal template
    if(d_thisScanIsCal)
		return d_calTemplate;

	//create new scan with the appropriate ft frequency
    double freq = d_surveyTemplate.ftFreq() + (double)d_currentSurveyIndex*d_step;
	Scan out(d_surveyTemplate);
	out.setFtFreq(freq);
    return out;
}

bool BatchSurvey::isBatchComplete()
{
    if(d_currentSurveyIndex == d_totalSurveyScans && !d_thisScanIsCal)
		return true;

	return false;
}


void BatchSurvey::processScan(Scan s)
{
	//if nextScanIsCal is true, that means the incoming scan was a calibration; so the next one will not be
	//otherwise, survey index should be incremented
    bool thisScanWasCal = d_thisScanIsCal;
    if(d_thisScanIsCal)
	{

	}
	else
	{


	}

    //Probably don't need to fit calibration scans!
    if(!thisScanWasCal)
        d_fitter->doFit(s);

	//calculate starting and ending frequencies that we will display, keeping in mind we might be scanning down
	//if the step size is too big, there will be gaps!
	QVector<QPointF> ft = d_fitter->doStandardFT(s.fid()).first;
	QList<QVector<QPointF> > out;
    bool badTune = s.tuningVoltage() < 0;

	if(thisScanWasCal)
	{
        d_thisScanIsCal = false;
        d_calScanNumbers.append(s.number());

		double max = 0.0;
		for(int i=0; i<ft.size(); i++)
			max = qMax(max,ft.at(i).y());

		//we want to plot this at the point at which it's taken.
		//if it's the first scan, we want to plot it at the first data point of the first scan
		//otherwise, we want to plot it at the last data point of the previous scan
		//this depends on whether we're scanning up or down
		double displayFreq;
        if(d_surveyData.isEmpty())
		{
			if(d_step > 0.0)
				displayFreq = d_surveyTemplate.ftFreq() - d_offset + d_chunkStart;
			else
				displayFreq = d_surveyTemplate.ftFreq() - d_offset + d_chunkEnd;
		}
		else
            displayFreq = d_surveyData.at(d_surveyData.size()-1).x();

        d_calData.append(QPointF(displayFreq,max));
        out.append(d_calData);

        BatchPlotMetaData md(type(),s.number(),displayFreq,displayFreq,true,badTune);

		emit plotData(md, out);
		return;
	}
	else
	{
        d_surveyScanNumbers.append(s.number());
        d_currentSurveyIndex++;

		double startFreq = s.fid().probeFreq() + d_chunkStart;
		double endFreq = s.fid().probeFreq() + d_chunkEnd;
        BatchPlotMetaData md(type(),s.number(),startFreq,endFreq,false,badTune);

		//if we're scanning down, the survey data will be ordered from highest to lowest frequency
		if(d_step < 0.0)
		{
			for(int i=0; i<ft.size(); i++)
			{
				double x = ft.at(ft.size()-1-i).x();
				if(x >= startFreq && x < endFreq)
                    d_surveyData.append(ft.at(ft.size()-1-i));
			}
		}
		else
		{
			for(int i=0; i<ft.size(); i++)
			{
				double x = ft.at(i).x();
				if(x >= startFreq && x < endFreq)
                    d_surveyData.append(ft.at(i));
			}
		}

        out.append(d_surveyData);
		emit plotData(md, out);

        if(d_hasCalibration)
        {
            if(d_currentSurveyIndex == d_totalSurveyScans)
            {
                //we've finished the survey; take a cal scan
                d_thisScanIsCal = true;
            }
            else if(d_scansPerCal > 0 && (d_currentSurveyIndex % d_scansPerCal) == 0)
            {
                //it's time to take a cal scan
                d_thisScanIsCal = true;
            }
        }
	}

}

void BatchSurvey::writeReport()
{
    if(d_surveyData.isEmpty())
	{
		emit logMessage(QString("Did not create survey report because no scans were completed."),QtFTM::LogWarning);
		return;
	}

	//figure out where to save survey
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	int surveyNum = s.value(d_numKey,1).toInt();
	int surveyMillions = (int)floor((double)surveyNum/1000000.0);
	int surveyThousands = (int)floor((double)surveyNum/1000.0);
	QDir d(QString("/home/data/QtFTM/surveys/%1/%2").arg(surveyMillions).arg(surveyThousands));

	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
		{
			emit logMessage(QString("Could not create directory for saving survey! Creation of %1 failed, and data were not saved!").arg(d.absolutePath()),QtFTM::LogError);
			emit fatalSaveError();
			return;
		}
	}

	//open file for writing
	QFile out(QString("%1/%2.txt").arg(d.absolutePath()).arg(surveyNum));
	QTextStream t(&out);
	QString tab = QString("\t");
	QString nl = QString("\n");

	if(!out.open(QIODevice::WriteOnly))
	{
		emit logMessage(QString("Could not open file for writing batch data! Creation of %1 failed, and data were not saved!").arg(out.fileName()),QtFTM::LogError);
		emit fatalSaveError();
		return;
	}

	//write header
	t << makeHeader(surveyNum);
	t.setRealNumberNotation(QTextStream::ScientificNotation);

	//this controls how many digits are printed after decimal.
	//in principle, an 8 bit digitizer requires only 3 digits (range = -127 to 128)
	//For each ~factor of 10 averages, we need ~one more digit of precision
	//This starts at 7 digits (sci notation; 6 places after decimal), and adds 1 for every factor of 10 shots.
	int logFactor = 0;
	if(d_surveyTemplate.targetShots() > 0)
		logFactor = (int)floor(log10((double)d_surveyTemplate.targetShots()));
	t.setRealNumberPrecision(6+logFactor);

	//write survey data
	t << nl << QString("surveyfreq") << surveyNum << QString("\tsurveyint") << surveyNum;
    for(int i=0; i<d_surveyData.size(); i++)
        t << nl << QString::number(d_surveyData.at(i).x(),'f',4) << tab << d_surveyData.at(i).y();

	//write survey scan numbers
	t << nl << nl << QString("surveyscans") << surveyNum;
    for(int i=0; i<d_surveyScanNumbers.size(); i++)
        t << nl << d_surveyScanNumbers.at(i);

	//write cal data
    if(d_totalCalScans > 0)
	{
		t << nl << nl << QString("surveycalscans") << surveyNum << tab << QString("surveycalfreq")
		  << surveyNum << tab << QString("surveycalint") << surveyNum;

        for(int i=0; i<d_calScanNumbers.size(); i++)
            t << nl << d_calScanNumbers.at(i) << tab << QString::number(d_calData.at(i).x(),'f',4) << tab << d_calData.at(i).y();
	}

	t.flush();
	out.close();

	//increment survey number
	s.setValue(d_numKey,surveyNum+1);
	s.sync();

}

QString BatchSurvey::makeHeader(int num)
{
	QString out;
	QTextStream t(&out);
	t.setRealNumberNotation(QTextStream::FixedNotation);
	t.setRealNumberPrecision(4);

	t << QString("#Survey\t") << num << QString("\t\n");
	t << QString("#Date\t") << QDateTime::currentDateTime().toString() << QString("\t\n");
    t << QString("#Start freq\t") << d_surveyData.at(0).x() << QString("\tMHz\n");
    t << QString("#End freq\t") << d_surveyData.at(d_surveyData.size()-1).x() << QString("\tMHz\n");
	t << QString("#Step size\t") << d_step << QString("\tMHz\n");
    t << QString("#Survey scans\t") << d_surveyScanNumbers.size() << QString("\t\n");
    t << QString("#Cal scans\t") << d_calScanNumbers.size() << QString("\t\n");
	t << QString("#Scans per cal\t") << d_scansPerCal << QString("\t\n");
	t << QString("#FID delay\t") << d_fitter->delay() << QString("\tus\n");
	t << QString("#FID high pass\t") << d_fitter->hpf() << QString("\tkHz\n");
	t << QString("#FID exp decay\t") << d_fitter->exp() << QString("\tus\n");
	t << QString("#FID remove baseline\t") << (d_fitter->removeDC() ? QString("Yes") : QString("No")) << QString("\t\n");
	t << QString("#FID zero padding\t") << (d_fitter->autoPad() ? QString("Yes") : QString("No")) << QString("\t\n");

	t.flush();
	return out;
}
