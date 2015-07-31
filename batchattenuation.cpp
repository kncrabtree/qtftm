#include "batchattenuation.h"

BatchAttenuation::BatchAttenuation(double minFreq, double maxFreq, double stepSize, int atten10GHz, Scan s, QString name) :
    BatchManager(QtFTM::Attenuation), d_scanUpComplete(false), d_scanDownComplete(false), d_scanUpIndex(0), d_scanDownIndex(0), d_template(s), d_atnFilename(name),
    d_minFreq(minFreq), d_maxFreq(maxFreq), d_stepSize(stepSize), d_atten10GHz(atten10GHz), d_retrying(false), d_aborted(false), d_nextAttn(atten10GHz)
{
    d_prettyName = QString("Attenuation Table Batch");

    if(d_stepSize < 1.0)
        d_stepSize = 1.0;

    //calculate number of scans and set equal to total shots
    //the first will be at 10 GHz, then will scan up until the max is hit
    //then we will return to 10 GHz, remeasure, and go down until the minimum
    int numScans = (int)ceil(fabs(d_maxFreq-10030.0)/d_stepSize) + 1;
    numScans += (int)ceil(fabs(10030.0-d_minFreq)/d_stepSize) + 1;

    d_totalShots = numScans;

    //create vectors with reserved memory for plot data
    for(int i=0; i<4; i++)
    {
        QVector<QPointF> res;
        res.reserve(d_totalShots);
        d_plotData.append(res);
    }
}

BatchAttenuation::BatchAttenuation(int num) :
    BatchManager(QtFTM::Attenuation,true)
{
    d_batchNum = num;
    d_prettyName = QString("Attenuation Table Batch");

    int millions = (int)floor((double)num/1000000.0);
    int thousands = (int)floor((double)num/1000.0);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QFile f(savePath + QString("/attn/%1/%2/%3.txt").arg(millions).arg(thousands).arg(num));

    if(!f.exists())
        return;

    if(!f.open(QIODevice::ReadOnly))
        return;

    //process header
    while(!f.atEnd())
    {
        QString line = f.readLine();
        if(!line.startsWith(QChar('#')))
            break;

        if(line.startsWith(QString("#Start")))
        {
            QStringList l = line.split(QString("\t"));
            if(l.size()<2)
                return;
            bool ok = false;
            d_minFreq = l.at(1).toDouble(&ok);
            if(!ok)
                return;
        }

        if(line.startsWith(QString("#End")))
        {
            QStringList l = line.split(QString("\t"));
            if(l.size()<2)
                return;
            bool ok = false;
            d_maxFreq = l.at(1).toDouble(&ok);
            if(!ok)
                return;
        }
    }

    while(!f.atEnd())
    {
        QString line = f.readLine();
        if(line.startsWith(QString("attnbatchfreq")))
            break;
    }

    while(!f.atEnd())
    {
        QString line = f.readLine();
        QStringList l = line.split(QString("\t"));
        if(l.size() < 5)
            continue;

        Scan s;
        s.setDummy();
        s.setFtFreq(l.at(0).trimmed().toDouble());
        s.setAttenuation(l.at(1).trimmed().toInt());
        s.setTuningVoltage(l.at(2).trimmed().toInt());

        d_loadAttnList.append(s);
    }

    d_retrying = false;
    d_scanUpComplete = false;
    d_scanDownComplete = false;
    d_scanUpIndex = 0;
    d_scanDownIndex = 0;

    f.close();

    //create vectors with reserved memory for plot data
    for(int i=0; i<4; i++)
    {
        QVector<QPointF> res;
        res.reserve(d_totalShots);
        d_plotData.append(res);
    }
}


Scan BatchAttenuation::prepareNextScan()
{
    Scan out(d_template);
    out.setDummy();
    out.setAttenuation(d_nextAttn);

    //first, we're scanning up
    if(!d_scanUpComplete)
    {
        if(d_scanUpIndex > 0)
            out.setFtFreq(qMin(d_maxFreq,10030.0 + (double)d_scanUpIndex*d_stepSize));
        else
            out.setFtFreq(10030.0);
    }
    else // we must be scanning down
    {
        if(d_scanDownIndex > 0)
            out.setFtFreq(qMax(d_minFreq,10030.0 - (double)d_scanDownIndex*d_stepSize));
        else
            out.setFtFreq(10030.0);
    }

    return out;
}

bool BatchAttenuation::isBatchComplete()
{
    return ((d_scanUpComplete && d_scanDownComplete) || d_aborted);
}

void BatchAttenuation::advanceBatch(const Scan s)
{
    //first, we need to assess whether the tuning was successful
    TuningValues tv(s);
    if(!tv.isValid())
    {
        //the tune failed. We can retry once; perhaps with less attenuation
        if(!d_retrying) // make sure this isn't already the second attempt!
        {
            d_retrying = true;
            d_nextAttn = qMax(d_nextAttn-10,0);
        }
        else // this is the second attempt, so we'll skip this one and move on
        {
            d_retrying = false;
            emit logMessage(QString("Tuning was unsuccessful after 2 tries at %1 MHz. This frequency was skipped.").arg(s.ftFreq(),0,'f',3),QtFTM::LogWarning);
        }
    }
    else
    {
        if(tv.tuningVoltage > 3000)
        {
            d_retrying = true;
            d_nextAttn = d_nextAttn+10;
        }
        else if(tv.tuningVoltage < 300 && tv.tuningAttn > 0)
        {
            d_retrying = true;
            d_nextAttn = qMax(0,d_nextAttn-5);
        }
        else
        {
            d_retrying = false;
            d_nextAttn = tv.correctedAttn;

          QtFTM::BatchPlotMetaData md(QtFTM::Attenuation,-1,s.ftFreq()-0.0001,s.ftFreq()+0.0001,false);
            //all good. Record data and send to plot unless this is a duplicate!
            if(!d_scanUpComplete)
            {
                d_tuningValues.append(tv);
                d_plotData[0].append(QPointF(s.ftFreq(),(double)tv.tuningAttn));
                d_plotData[1].append(QPointF(s.ftFreq(),(double)tv.tuningVoltage));
                d_plotData[2].append(QPointF(s.ftFreq(),(double)tv.correctedAttn));
                d_plotData[3].append(QPointF(s.ftFreq(),(double)tv.correctedVoltage));
                emit plotData(md,d_plotData);
            }
            else if(d_scanDownIndex > 0) //don't record or send the duplicate 10 GHz tune!
            {
                d_tuningValues.prepend(tv);
                d_plotData[0].prepend(QPointF(s.ftFreq(),(double)tv.tuningAttn));
                d_plotData[1].prepend(QPointF(s.ftFreq(),(double)tv.tuningVoltage));
                d_plotData[2].prepend(QPointF(s.ftFreq(),(double)tv.correctedAttn));
                d_plotData[3].prepend(QPointF(s.ftFreq(),(double)tv.correctedVoltage));
                emit plotData(md,d_plotData);
            }
        }
    }

    //update internal variables
    if(!d_retrying)
    {
        if(!d_scanUpComplete)
        {
            d_scanUpIndex++;
            if(tv.frequency >= d_maxFreq)
            {
                d_scanUpComplete = true;
                d_nextAttn = d_atten10GHz;
            }
        }
        else
        {
            d_scanDownIndex++;
            if(tv.frequency <= d_minFreq)
                d_scanDownComplete = true;
        }

        emit elementComplete();
    }

}

void BatchAttenuation::processScan(Scan s)
{
    //nothing to do; processing had to be done in advance in order to determine next scan, and there is no line to fit
    Q_UNUSED(s)
}

void BatchAttenuation::writeReport()
{
    if(d_tuningValues.isEmpty())
    {
        emit logMessage(QString("Did not create %1 report because no tunings were completed successfully.").arg(d_prettyName),QtFTM::LogWarning);
        return;
    }

    //figure out where to save the data
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    int num = s.value(d_numKey,1).toInt();
    int millions = (int)floor((double)num/1000000.0);
    int thousands = (int)floor((double)num/1000.0);

    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/attn/%1/%2").arg(millions).arg(thousands));

    if(!d.exists())
    {
        if(!d.mkpath(d.absolutePath()))
        {
            emit logMessage(QString("Could not create directory for saving %1! Creation of %2 failed, and data were not saved!").arg(d_prettyName).arg(d.absolutePath()),QtFTM::LogError);
            return;
        }
    }

    //create batch output file
    QFile out(QString("%1/%2.txt").arg(d.absolutePath()).arg(num));
    QTextStream t(&out);
    QString tab = QString("\t");
    QString nl = QString("\n");

    if(!out.open(QIODevice::WriteOnly))
    {
        emit logMessage(QString("Could not open file for writing %1 data! Creation of %2 failed, and data were not saved!").arg(d_prettyName).arg(out.fileName()),QtFTM::LogError);
        return;
    }

    QDir d2(savePath + QString("/tuningTables"));
    if(!d2.exists())
    {
        if(!d2.mkpath(d2.absolutePath()))
        {
            emit logMessage(QString("Could not create directory for saving attenuation file! Creation of %1 failed, and data were not saved!").arg(d2.absolutePath()),QtFTM::LogError);
            return;
        }
    }

    //create attenuation table output file
    QFile atnFile(savePath + QString("/tuningTables/%1.atn").arg(d_atnFilename));
    if(atnFile.exists()) // don't overwrite an existing file. make a new one!
    {
        bool done = false;
        int suffixIndex = 1;
        while(!done)
        {
            QString testName = QString("%1_%2").arg(d_atnFilename).arg(suffixIndex);
		  atnFile.setFileName(savePath + QString("/tuningTables/%1.atn").arg(testName));
            if(!atnFile.exists())
                done = true;
            else
                suffixIndex++;
        }
    }

    bool writeAtnFile = true;
    if(!atnFile.open(QIODevice::WriteOnly))
    {
        writeAtnFile = false;
        emit logMessage(QString("Could not open .atn file for writing (%1)").arg(atnFile.fileName()),QtFTM::LogWarning);
    }


    //create header
    t << QString("#") << d_prettyName << tab << num << tab << nl;
    t << QString("#Date") << tab << QDateTime::currentDateTime().toString() << tab << nl;
    t << QString("#Start freq") << tab << QString::number(d_tuningValues.at(0).frequency,'f',3) << QString("\tMHz\n");
    t << QString("#End freq") << tab << QString::number(d_tuningValues.at(d_tuningValues.size()-1).frequency,'f',3) << QString("\tMHz\n");
    t << QString("#Step size") << tab << QString::number(d_stepSize,'f',3) << QString("\tMHz\n");
    t << QString("#Attenuation at 10 GHz") << tab << d_atten10GHz << QString("\tdB\n");
    if(writeAtnFile)
        t << QString("#Attenuation file") << tab << atnFile.fileName() << tab << nl;
    else
        t << QString("#Attenuation file") << tab << QString("NONE") << tab << nl;

    t << nl;

    //column headers
    t << QString("attnbatchfreq") << num << tab
      << QString("attnbatchta") << num << tab
      << QString("attnbatchtv") << num << tab
      << QString("attnbatchca") << num << tab
      << QString("attnbatchcv") << num << tab;

    //data
    for(int i=0;i<d_tuningValues.size();i++)
        t << nl << d_tuningValues.at(i).toString();

    t.flush();
    out.close();

    if(writeAtnFile)
    {
        QTextStream ts(&atnFile);
        ts << QString("#Created by QtFTM") << tab << num;
        for(int i=0;i<d_tuningValues.size();i++)
            ts << nl << QString::number(d_tuningValues.at(i).frequency,'f',3) << tab << d_tuningValues.at(i).correctedAttn;

        ts.flush();

        atnFile.close();

        emit logMessage(QString("Attenuation file written to %1").arg(atnFile.fileName()),QtFTM::LogNormal);
    }

    s.setValue(d_numKey,num+1);
}
