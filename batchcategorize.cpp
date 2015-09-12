#include "batchcategorize.h"

#include <QStringList>

BatchCategorize::BatchCategorize(QList<QPair<Scan, bool> > scanList, QList<CategoryTest> testList, AbstractFitter *ftr) :
    BatchManager(QtFTM::Categorize,false,ftr), d_templateList(scanList)
{
	d_prettyName = QString("Category Test");

    //eliminate empty tests in test list
    for(int i=0; i<testList.size(); i++)
    {
        if(testList.at(i).valueList.size() > 0)
	   {
		   CategoryTest test = testList.at(i);
		   if((test.key == QString("dc") || test.key == QString("m")) && !d_testList.isEmpty())
		   {
			   //only need to do one test; which way it will be toggled will be determined
			   //automatically later
			   QVariantList l{ true };
			   test.valueList = l;
		   }
		  d_testList.append(test);
	   }
    }

    //can't calculate total shots. use list size and advance manually
    d_totalShots = d_templateList.size();

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("attn"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    d_maxAttn = s.value(QString("max"),100).toInt();
    s.endGroup();
    s.endGroup();
}

BatchCategorize::BatchCategorize(int num, AbstractFitter *ftr) :
    BatchManager(QtFTM::Categorize,true,ftr)
{
    d_batchNum = num;

    int catNum = num;
    int catMillions = (int)floor((double)catNum/1000000.0);
    int catThousands = (int)floor((double)catNum/1000.0);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/categorize/%1/%2").arg(catMillions).arg(catThousands));

    //create output file
    QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(catNum));

    if(!f.exists())
        return;

    if(!f.open(QIODevice::ReadOnly))
        return;

    //don't need anyting from header; read until we get to the column headers
    while(!f.atEnd())
    {
        QString line = QString(f.readLine().trimmed());
        if(line.startsWith(QString("id_")))
            break;
    }

    while(!f.atEnd())
    {
        QString line = QString(f.readLine().trimmed());
        if(line.isEmpty())
            continue;

        if(line.startsWith(QString("cal")))
            break;

        QStringList l = line.split(QString("\t"));
        if(l.size() < 8)
            continue;

        int num = l.at(1).toInt();
        if(num < 1)
            continue;

	   d_loadScanList.append(num);

        QString test = l.at(2);
        QString value = l.at(3);
        QString labelText;
        if(test.contains(QString("sat")))
            labelText = QString("SAT");
        else if(test.contains(QString("final")))
        {
            labelText = QString("FINAL");
            QStringList cats = value.split(QString("/"));
            if(cats.size() < 2)
                labelText.append(QString("\n")).append(value);
            else
            {
                for(int i=0; i<cats.size(); i++)
                {
                    if(i%2)
                        labelText.append(QString("\n"));
                    else
                        labelText.append(QString("/"));
                    labelText.append(cats.at(i));
                }
            }
        }
        else
            labelText = QString("%1:%2").arg(test).arg(value);

        d_loadLabelTextMap.insert(num,labelText);
    }

    while(!f.atEnd())
    {
        QString line = QString(f.readLine().trimmed());
        if(line.isEmpty())
            continue;

        bool ok = false;
        int num = line.toInt(&ok);
	   d_loadScanList.append(num);
        if(ok)
            d_loadLabelTextMap.insert(num,QString("CAL"));
    }

    std::sort(d_loadScanList.begin(),d_loadScanList.end());

}



BatchCategorize::~BatchCategorize()
{

}

void BatchCategorize::writeReport()
{
    if(d_resultList.isEmpty())
    {
        emit logMessage(QString("Did not create %1 report because no scans were completed successfully.").arg(d_prettyName),QtFTM::LogWarning);
        return;
    }

    //figure out where to save the data
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    int batchNum = s.value(d_numKey,1).toInt();
    int batchMillions = (int)floor((double)batchNum/1000000.0);
    int batchThousands = (int)floor((double)batchNum/1000.0);

    //create directory, if necessary
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/categorize/%1/%2").arg(batchMillions).arg(batchThousands));
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

    if(!out.open(QIODevice::WriteOnly))
    {
        emit logMessage(QString("Could not open file for writing batch data! Creation of %1 failed, and data were not saved!").arg(out.fileName()),QtFTM::LogError);
        return;
    }

    t.setRealNumberNotation(QTextStream::FixedNotation);
    t.setRealNumberPrecision(4);
    //header stuff
    t << QString("#Category Test") << tab << batchNum << tab << nl;
    t << QString("#Date") << tab << QDateTime::currentDateTime().toString() << tab << nl;
    t << QString("#FID delay") << tab << d_fitter->delay() << tab << QString("us") << nl;
    t << QString("#FID high pass") << tab << d_fitter->hpf() << tab << QString("kHz") << nl;
    t << QString("#FID exp decay") << tab << d_fitter->exp() << tab << QString("us") << nl;
    t << QString("#FID remove baseline") << tab << (d_fitter->removeDC() ? QString("Yes") : QString("No")) << tab << nl;
    t << QString("#FID zero padding") << tab << (d_fitter->autoPad() ? QString("Yes") : QString("No")) << tab << nl;

    t << nl << QString("id_") << batchNum << tab << QString("scan_") << batchNum << tab;
    t << QString("test_") << batchNum << tab << QString("value_") << batchNum << tab;
    t << QString("extraAttn_") << batchNum << tab << QString("attn_") << batchNum << tab;
    t << QString("ftMax_") << batchNum << tab;
    t << QString("lines_") << batchNum << tab << QString("intensities_") << batchNum << tab;
    t << QString("afFreq_") << batchNum << tab << QString("afInt_") << batchNum;

    QString slash("/"), zero("0.0");
    for(int i=0; i<d_resultList.size(); i++)
    {
        const ScanResult &sr = d_resultList.at(i);
        t << nl << sr.index << tab << sr.scanNum << tab << sr.testKey << tab << sr.testValue.toString();
        t << tab << sr.extraAttn << tab << sr.attenuation << tab;
        t << sr.ftMax << tab;
	   if(sr.frequencies.isEmpty())
		  t << zero;
        else
        {
		  t << sr.frequencies.first();
		  for(int i=1; i<sr.frequencies.size(); i++)
			 t << slash << sr.frequencies.at(i);
        }
        t << tab;
        if(sr.intensities.isEmpty())
		  t << zero;
        else
        {
            t << sr.intensities.first();
            for(int i=1; i<sr.intensities.size(); i++)
			 t << slash << sr.intensities.at(i);
        }
	   t << tab;
	   if(sr.fit.type() == FitResult::NoFitting || sr.fit.freqAmpPairList().isEmpty())
		   t << zero;
	   else
	   {
		   t << sr.fit.freqAmpPairList().first().first;
		   for(int i=1; i<sr.fit.freqAmpPairList().size(); i++)
			   t << slash << sr.fit.freqAmpPairList().at(i).first;
	   }
	   t << tab;
	   if(sr.fit.type() == FitResult::NoFitting || sr.fit.freqAmpPairList().isEmpty())
		   t << zero;
	   else
	   {
		   t << sr.fit.freqAmpPairList().first().second;
		   for(int i=1; i<sr.fit.freqAmpPairList().size(); i++)
			   t << slash << sr.fit.freqAmpPairList().at(i).second;
	   }
    }

    if(!d_calScans.isEmpty())
    {
        t << nl << nl << QString("calscans_") << batchNum;
        for(int i=0; i<d_calScans.size(); i++)
            t << nl << d_calScans.at(i);
    }

    t.flush();
    out.close();

    s.setValue(d_numKey,batchNum+1);
    s.sync();


}

void BatchCategorize::advanceBatch(const Scan s)
{
    //in order to determine what scan to do next, we need to do the processing here
    //instead of in processScan

    FitResult res;
    if(d_loading && d_fitter->type() == FitResult::NoFitting)
        res = FitResult(s.number());
    else
        res = d_fitter->doFit(s);

    auto p = d_fitter->doStandardFT(s.fid());
    QVector<QPointF> ft = p.first;
    double max = p.second;

    double mdMin = (double)s.number() - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9;
    double mdMax = (double)s.number() - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9;
    bool badTune = (s.tuningVoltage() <= 0);
    QString labelText;
    bool isRef = false;

    if(d_loading)
    {
        QString lt = d_loadLabelTextMap.value(s.number());
        if(lt.contains(QString("CAL")))
        {
            d_thisScanIsCal = true;
            labelText = lt;
        }
        else
            d_thisScanIsCal = false;
    }


    if(d_thisScanIsCal)
    {
        double intensity = 0.0;
        if(res.freqAmpPairList().isEmpty())
            intensity = p.second;
        else
        {
            //record strongest peak intensity
            for(int i=0; i<res.freqAmpPairList().size(); i++)
                intensity = qMax(intensity,res.freqAmpPairList().at(i).second);
        }
        mdMin = (double)s.number();
        mdMax = (double)s.number();

        d_calData.append(QPointF(static_cast<double>(s.number()),intensity));
        labelText = QString("CAL");

        d_calScans.append(s.number());
        d_status.advance();
        emit advanced();
    }
    else
    {
        double num = (double)s.number();

        d_status.currentAttn = s.attenuation();
        bool isSaturated;
        if(d_fitter->type() == FitResult::NoFitting)
            isSaturated = d_fitter->isFidSaturated(s);
        else
            isSaturated = (res.category() == FitResult::Saturated);

        //the data will start and end with a 0 to make the plot look a little nicer
        d_scanData.append(QPointF(num - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9,0.0));
        for(int i=0; i<ft.size(); i++) // make the x range go from num - 0.45 to num + 0.45
            d_scanData.append(QPointF(num - ((double)ft.size()/2.0 - (double)i)/(double)ft.size()*0.9,ft.at(i).y()));
        d_scanData.append(QPointF(num - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9,0.0));

        labelText.append(QString("%1/%2\n").arg(s.ftFreq(),0,'f',3).arg(s.attenuation()));
        if(d_loading)
        {
            QString lt = d_loadLabelTextMap.value(s.number());
            if(lt.contains(QString("FINAL")))
                isRef = true;
            labelText.append(lt);
        }
        else
        {
            d_status.scanTemplate.setAttenuation(s.attenuation());
            //build scan result; make initial settings.
            //some will be overwritten later as the scan is processed
            d_status.scansTaken++;
            ScanResult sr;
            sr.index = d_status.scanIndex;
            sr.scanNum = s.number();
            sr.extraAttn = d_status.currentExtraAttn;
            sr.attenuation = s.attenuation();
            sr.testKey = d_status.currentTestKey;
            sr.testValue = d_status.currentTestValue;
		  sr.frequencies = d_status.frequencies;
            sr.ftMax = max;
		  sr.fit = res;

            if(res.type() == FitResult::NoFitting)
            {
			 for(int i=0; i<d_status.frequencies.size(); i++)
                {
                    while(i >= sr.intensities.size())
                        sr.intensities.append(0.0);
                    sr.intensities[i] = max;
                }
            }
            else
            {

			 for(int i=0; i<sr.frequencies.size(); i++)
                {
				double thisLineFreq = sr.frequencies.at(i);
                    while(i >= sr.intensities.size())
                        sr.intensities.append(0.0);
                    sr.intensities[i] = 0.0;
                    for(int j=0; j<res.freqAmpPairList().size(); j++)
                    {
                        if(qAbs(thisLineFreq-res.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff)
                            sr.intensities[i] = res.freqAmpPairList().at(j).second;
                    }
                }
            }

            TestResult tr;
            tr.scanNum = s.number();
            tr.key = d_status.currentTestKey;
            tr.value = d_status.currentTestValue;
            tr.result = res;
            tr.extraAttn = d_status.currentExtraAttn;
            tr.ftMax = max;

            //use results to determine what scan to do next
            //1.) If saturated, increase attenuation and try again. Remove dipole test if applicable
            //2.) If this is the reference scan, assign category and move on to next scan in list
            //3.) Check for line. If no line, proceed with any dipole tests remaining. Otherwise, continue.
            //4.) Proceed through tests.

            //for any result, we need to configure the ScanResult, possibly work with the TestResult, and make the labelText
            if(isSaturated)
            {
                //if saturated, don't waste time scanning. 20 shots should be ok
                d_status.scanTemplate.setTargetShots(20);
                sr.testKey = QString("sat");
                sr.testValue = d_status.currentExtraAttn;
                if(d_status.currentAttn + 10 >= d_maxAttn)
                {
                    sr.testKey = QString("final");
                    sr.testValue = QString("SAT");
                    isRef = true;
                    labelText.append(QString("FINAL\nSAT"));
                    d_status.advance();
                    emit advanced();
                }
                else
                {
                    d_status.currentExtraAttn += 10;
                    d_status.scanTemplate.setAttenuation(d_status.currentAttn+10);
                    d_status.scanTemplate.setDipoleMoment(0.0);
                    labelText.append(QString("SAT"));
                }
            }
            else
            {
                if(d_status.currentTestKey == QString("u") && d_fitter->type() != FitResult::NoFitting)
                {
                    //during the dipole test, we will check for new lines and eliminate false positives if using the autofitter
                    //the line closest to the ft frequency comes first
                    for(int i=0; i<res.freqAmpPairList().size(); i++)
                    {
                        double thisLineFreq = res.freqAmpPairList().at(i).first;
                        double thisLineInt = res.freqAmpPairList().at(i).second;
				    bool addToFreqList = (thisLineInt > 10.0);

                        if(!addToFreqList) //we are skeptical of this line's intensity, so we'd like to see it again
                        {
                            //look in our current list of candidates. If there is a match, add it to the frequency list
                            for(int j=0; j<d_status.candidates.size(); j++)
                            {
                                double candidateFreq = d_status.candidates.at(j).frequency;
                                if(qAbs(thisLineFreq - candidateFreq) < d_lineMatchMaxDiff)
                                {
                                    d_status.candidates.removeAt(j);
                                    addToFreqList = true;
                                    break;
                                }
                            }

                            //if addToFreqList is still false, then store this in the list of candidates for later
                            if(!addToFreqList)
                            {
                                Candidate newCandidate;
                                newCandidate.frequency = thisLineFreq;
                                newCandidate.intensity = thisLineInt;
                                newCandidate.dipole = d_status.currentTestValue.toDouble();
                                d_status.candidates.append(newCandidate);
                            }

                        }

                        if(addToFreqList)
                        {
                            //make sure this line isn't already there
					   for(int j=0; j<d_status.frequencies.size(); j++)
                            {
						  if(qAbs(d_status.frequencies.at(j)-thisLineFreq) < d_lineMatchMaxDiff)
                                    addToFreqList = false;
                            }

                            if(addToFreqList)
                            {
						  if(d_status.frequencies.isEmpty())
                                {
							 d_status.frequencies.append(thisLineFreq);
                                    sr.intensities.append(thisLineInt);
                                }
						  else if(qAbs(d_status.frequencies.first()-s.ftFreq()) < qAbs(thisLineFreq-s.ftFreq()))
                                {
							 d_status.frequencies.prepend(thisLineFreq);
                                    sr.intensities.prepend(thisLineInt);
                                }
                                else
                                {
							 d_status.frequencies.append(thisLineFreq);
                                    sr.intensities.append(thisLineInt);
                                }
                            }
                        }
                    }
				if(d_status.currentTestValue.toString() == QString("check"))
					d_status.candidates.clear();
                }
			 else if(d_status.frequencies.isEmpty())
                {
                    //check to see if lines are present
                    //if fitter is not being used, then we just say 1 line is here and go with the FT Max
                    if(d_fitter->type() == FitResult::NoFitting)
                    {
				    d_status.frequencies.append(s.ftFreq());
                        sr.intensities.append(max);
                    }
                    else
                    {
                        //the line closest to the ft frequency comes first
                        for(int i=0; i<res.freqAmpPairList().size(); i++)
                        {
					   if(d_status.frequencies.isEmpty())
                            {
						  d_status.frequencies.append(res.freqAmpPairList().at(i).first);
                                sr.intensities.append(res.freqAmpPairList().at(i).second);
                            }
					   else if(qAbs(res.freqAmpPairList().at(i).first-s.ftFreq()) < qAbs(d_status.frequencies.first()-s.ftFreq()))
                            {
						  d_status.frequencies.prepend(res.freqAmpPairList().at(i).first);
                                sr.intensities.prepend(res.freqAmpPairList().at(i).second);
                            }
                            else
                            {
						  d_status.frequencies.append(res.freqAmpPairList().at(i).first);
                                sr.intensities.append(res.freqAmpPairList().at(i).second);
                            }
                        }
                    }
                }

			 sr.frequencies = d_status.frequencies;

                if(d_status.currentTestKey == QString("final"))
                {
                    //categorize
                    sr.testValue = d_status.category;
                    if(d_status.category.isEmpty())
                    {
                        sr.testValue = QString("noCat");
				    if(!d_status.frequencies.isEmpty())
                        {
					   for(int i=1; i<d_status.frequencies.size(); i++)
                                sr.testValue = QString(sr.testValue.toString()).append(QString("/noCat"));
                        }
                    }
                    labelText.append(QString("FINAL"));
                    QStringList cats = d_status.category.split(QString("/"));
                    if(cats.size() < 2)
                        labelText.append(QString("\n")).append(d_status.category);
                    else
                    {
                        for(int i=0; i<cats.size(); i++)
                        {
                            if(i%2)
                                labelText.append(QString("\n"));
                            else
                                labelText.append(QString("/"));
                            labelText.append(cats.at(i));
                        }
                    }
                    isRef = true;
                    d_status.advance();
                    emit advanced();
                }
                else
                {
                    if(d_status.currentTestKey == QString("u"))
                    {
				    if((d_status.currentTestValue.toString() == QString("check") || (d_status.currentValueIndex+1 >= d_testList.at(d_status.currentTestIndex).valueList.size() && d_status.candidates.isEmpty()))
						    && d_status.frequencies.isEmpty())
                        {
                            sr.testKey = QString("final");
                            sr.testValue = QString("ND");
                            isRef = true;
                            labelText.append(QString("FINAL\nND"));
                            d_status.advance();
                            emit advanced();
                        }
                        else if(d_status.currentExtraAttn > 0)
                        {
                            if(!skipCurrentTest())
                            {
                                sr.testKey = QString("final");
                                sr.testValue = QString("SAT");
                                isRef = true;
                                labelText.append(QString("FINAL\nSAT"));
                                d_status.advance();
                                emit advanced();
                            }
                        }
				    else if((d_status.currentValueIndex+1 >= d_testList.at(d_status.currentTestIndex).valueList.size() && !d_status.candidates.isEmpty()) && d_status.frequencies.isEmpty())
                        {
                            d_status.currentTestValue = QString("check");
                            //check dipole of strongest candidate
                            double dipole = d_status.candidates.first().dipole;
                            double bestInt = d_status.candidates.first().intensity;
                            for(int i=1; i<d_status.candidates.size(); i++)
                            {
                                if(d_status.candidates.at(i).intensity > bestInt)
                                {
                                    bestInt = d_status.candidates.at(i).intensity;
                                    dipole = d_status.candidates.at(i).dipole;
                                }
                            }
                            d_status.scanTemplate.setDipoleMoment(dipole);
                        }
                        else
                        {
                            if(d_status.currentTestValue == QString("check"))
					   {
                                d_status.currentTestValue = s.dipoleMoment();
						  tr.value = s.dipoleMoment();
						  sr.testValue = s.dipoleMoment();
					   }

                            labelText.append(QString("%1:%2").arg(d_status.currentTestKey).arg(d_status.currentTestValue.toString()));
                            d_status.resultMap.insertMulti(d_status.currentTestKey,tr);
                            setNextTest();
                        }
                    }
                    else
                    {
				    //check to see if the frequency list is still empty. if so, this is a non detection
				    if(d_status.frequencies.isEmpty())
                        {
                            sr.testKey = QString("final");
                            sr.testValue = QString("ND");
                            isRef = true;
                            labelText.append(QString("FINAL\nND"));
                            d_status.advance();
                            emit advanced();
                        }
                        else
                        {
                            labelText.append(QString("%1:%2").arg(d_status.currentTestKey).arg(d_status.currentTestValue.toString()));
                            d_status.resultMap.insertMulti(d_status.currentTestKey,tr);
                            setNextTest();
                        }
                    }
                }
            }

            d_resultList.append(sr);

        } // end if(d_loading) else
    } // end if(d_thisScanIsCal) else

    QtFTM::BatchPlotMetaData md(d_batchType,s.number(),mdMin,mdMax,d_thisScanIsCal,badTune,labelText);
    md.isRef = isRef;
    QList<QVector<QPointF>> out;
    if(d_thisScanIsCal)
        out.append(d_calData);
    else
        out.append(d_scanData);

    emit plotData(md,out);
}

void BatchCategorize::processScan(Scan s)
{
    Q_UNUSED(s)
}

Scan BatchCategorize::prepareNextScan()
{
    if(d_status.scansTaken == 0)
    {
        if(d_status.scanIndex < d_templateList.size())
        {
            d_status.scanTemplate = d_templateList.at(d_status.scanIndex).first;
            d_thisScanIsCal = d_templateList.at(d_status.scanIndex).second;
            if(!d_thisScanIsCal)
                configureScanTemplate();
        }
    }

    return d_status.scanTemplate;
}

bool BatchCategorize::isBatchComplete()
{
    return d_status.scanIndex >= d_templateList.size();
}

bool BatchCategorize::setNextTest()
{
    //return false if we need to advance to next template
    //don't crash if things aren't set correctly
    if(d_status.currentTestKey == QString("final"))
        return false;

    if(d_status.currentTestIndex >= d_testList.size())
    {
        d_status.currentTestKey = QString("final");
        return true;
    }

    //check if this test is complete
    if(d_status.currentValueIndex + 1 >= d_testList.at(d_status.currentTestIndex).valueList.size())
    {
        //reset index, check if all tests complete
        d_status.currentValueIndex = 0;

        //get best result for this test
        getBestResult();
        d_status.currentTestIndex++;
    }
    else
    {
        //increment value index
        //exception: if there is extra attenuation and the current test is a dipole test, go to next test
        if(d_status.currentTestKey == QString("u") && d_status.currentExtraAttn > 0)
        {
            //reset index, check if all tests complete
            d_status.currentValueIndex = 0;
            d_status.currentTestIndex++;
        }
        else
            d_status.currentValueIndex++;
    }

    if(d_status.currentTestIndex >= d_testList.size())
    {
        d_status.currentTestKey = QString("final");
        return true;
    }

    return configureScanTemplate();

}

bool BatchCategorize::skipCurrentTest()
{
    //return false if template should be advanced
    if(d_status.currentTestKey == QString("final"))
        return false;

    d_status.currentValueIndex = 0;
    d_status.currentTestIndex++;
    if(d_status.currentTestIndex >= d_testList.size())
    {
        if(d_status.currentTestKey == QString("u") && d_status.currentExtraAttn > 0)
            return false;

        d_status.currentTestKey = QString("final");
        return true;
    }

    d_status.scanTemplate.setDipoleMoment(0.0);
    d_status.scanTemplate.setMagnet(false);
    getBestResult();

    return configureScanTemplate();


}

bool BatchCategorize::configureScanTemplate()
{
    //return value indicates scan template should be advanced. Should never return false, but just in case...
    d_status.currentTestKey = d_testList.at(d_status.currentTestIndex).key;
    if(d_status.currentTestKey == QString("v"))
    {
        //skip voltage test if not a DC line
        if(!d_status.scanTemplate.pulseConfiguration().isDcEnabled())
            return skipCurrentTest();
        else
        {
            d_status.currentTestValue = d_testList.at(d_status.currentTestIndex).valueList.at(d_status.currentValueIndex);
            d_status.scanTemplate.setDcVoltage(d_status.currentTestValue.toInt());
        }
    }
    else if(d_status.currentTestKey == QString("dc"))
    {
        auto pc = d_status.scanTemplate.pulseConfiguration();
        bool en = !pc.isDcEnabled();
        d_status.currentTestValue = en;
        pc.setDcEnabled(en);
        d_status.scanTemplate.setPulseConfiguration(pc);
    }
    else if(d_status.currentTestKey == QString("m"))
    {
        bool en = !d_status.scanTemplate.magnet();
        d_status.currentTestValue = en;
        d_status.scanTemplate.setMagnet(en);
    }
    else if(d_status.currentTestKey == QString("u"))
    {
        d_status.currentTestValue = d_testList.at(d_status.currentTestIndex).valueList.at(d_status.currentValueIndex);
        d_status.scanTemplate.setDipoleMoment(d_status.currentTestValue.toDouble());
    }

    if(d_status.scansTaken == 0)
        d_status.scanTemplate.setSkiptune(false);
    else
        d_status.scanTemplate.setSkiptune(true);

    return true;
}

void BatchCategorize::getBestResult()
{
    //exception: magnet always stays off after (though, since it's usually last test, shoudln't matter)
    auto tests = d_status.resultMap.values(d_status.currentTestKey);

    int numLines = d_status.frequencies.size();
    //tests should not be empty. but if it is, we can probably just continue, leaving the scan template unchanged.
    //numLines should also be > 0 (this function should not get called if numLines = 0). if not, just continue
    if(!tests.isEmpty() && numLines > 0)
    {
        //for dc and mag tests, we need to compare on vs off
        //we may not have two tests with the on/off values in tests
        //might need to search for previous best value
        //when we find it, we'll insert it into the d_status resultsMap
	   if(d_status.currentTestKey == QString("dc") || d_status.currentTestKey == QString("m"))
        {
            TestResult tr;
            tr.key = d_status.currentTestKey;

            if(tests.size() == 1)
            {
                //if a voltage test was done, then we can find the fit result from the test with the same voltage
                if(d_status.resultMap.contains(QString("v")))
                {
                    auto vTests = d_status.resultMap.values(QString("v"));
                    for(int i=0; i<vTests.size(); i++)
                    {
                        const TestResult &vtr = vTests.at(i);
                        //the scan template has the "best" voltage
                        if(vtr.value.toInt() == d_status.scanTemplate.dcVoltage())
                        {
                            //the value from the voltage test is opposite the current value (because magnet was toggled)
                            tr.extraAttn = vtr.extraAttn;
                            tr.value = !tests.first().value.toBool();
                            tr.key = d_status.currentTestKey;
                            tr.ftMax = vtr.ftMax;
                            tr.result = vtr.result;
                            tr.scanNum = vtr.scanNum;

                        } // end if voltages match
                    } // end loop over voltage tests
                } // end if voltage tests
                else if(d_status.resultMap.contains(QString("u"))) //otherwise, there must be a dipole test, because there is only 1 dc/mag test if a previous test has been done
                {
                    tr.extraAttn = d_status.bestDipoleResult.extraAttn;
                    tr.key = d_status.currentTestKey;
                    tr.value = !tests.first().value.toBool();
                    tr.ftMax = d_status.bestDipoleResult.ftMax;
                    tr.result = d_status.bestDipoleResult.result;
                    tr.scanNum = d_status.bestDipoleResult.scanNum;
                }

                //at this point, we have made the testResult structure. put it in the status list, and reassign the tests variable
                d_status.resultMap.insertMulti(d_status.currentTestKey,tr);
                tests = d_status.resultMap.values(d_status.currentTestKey);
            }

            //now we better have two tests in the list!
            //otherwise, we'll just stupidly continue, because I don't think there's anything smarter that can be done......
            if(tests.size() == 2)
            {
                TestResult onTest, offTest;
                if(tests.first().value.toBool() == true)
                {
                    onTest = tests.first();
                    offTest = tests.last();
                }
                else
                {
                    onTest = tests.last();
                    offTest = tests.first();
                }


                QList<bool> resultList;
                bool dcOnOff = true;
                for(int i=0; i<numLines; i++)
                    resultList.append(false);
                if(d_status.currentTestKey == QString("dc"))
                {

                    if(onTest.result.type() == FitResult::NoFitting)
                    {
                        if(offTest.extraAttn != onTest.extraAttn)
                            resultList[0] = false;
                        else if(offTest.ftMax < d_dcThresh*onTest.ftMax)
                            resultList[0] = true;
                        else
                            resultList[0] = false;

                        if(offTest.ftMax > onTest.ftMax)
                            dcOnOff = false;
                    }
                    else
                    {
                        //if the "off" test contains a line at the given frequency, then it's NOT a DC line
                        for(int i=0; i<numLines; i++)
                        {
					   double thisLineFreq = d_status.frequencies.at(i);
                            double thisLineOnInt = 0.0;
                            for(int j=0; j<onTest.result.freqAmpPairList().size(); j++)
                            {
                                if(qAbs(thisLineFreq-onTest.result.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff)
                                {
                                    thisLineOnInt = onTest.result.freqAmpPairList().at(j).second;
                                    break;
                                }
                            }
                            resultList[i] = true;
                            for(int j=0; j<offTest.result.freqAmpPairList().size(); j++)
                            {
                                if(qAbs(thisLineFreq-offTest.result.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff)
                                {
                                    resultList[i] = false;
                                    if(thisLineOnInt < offTest.result.freqAmpPairList().at(j).second)
                                        dcOnOff = false;
                                    break;
                                }
                            }
                        }
                    }

                    //set next scan to DC on or off, depending on line closest to center (first in resultList)
                    PulseGenConfig pc = d_status.scanTemplate.pulseConfiguration();
                    pc.setDcEnabled(dcOnOff);
                    d_status.scanTemplate.setPulseConfiguration(pc);

                }//end if dc
                else
                {
                    if(onTest.result.type() == FitResult::NoFitting)
                    {
                        if(offTest.extraAttn != onTest.extraAttn)
                            resultList[0] = true;
                        else if(onTest.ftMax < d_magThresh*offTest.ftMax)
                            resultList[0] = true;
                        else
                            resultList[0] = false;
                    }
                    else
                    {
                        //if the "on" test does not contain the line, or if the line is weaker than it is in the "off" test, it IS magnetic
                        for(int i=0; i<numLines; i++)
                        {
					   double thisLineFreq = d_status.frequencies.at(i);
                            double thisLineOffInt = 0.0;
                            resultList[i] = true;
                            for(int j=0; j<offTest.result.freqAmpPairList().size(); j++)
                            {
                                if(qAbs(thisLineFreq-offTest.result.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff)
                                {
                                    thisLineOffInt = offTest.result.freqAmpPairList().at(j).second;
                                    break;
                                }
                            }
                            if(thisLineOffInt > 0.0)
                            {
                                for(int j=0; j<onTest.result.freqAmpPairList().size(); j++)
                                {
                                    if(qAbs(thisLineFreq-onTest.result.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff && onTest.result.freqAmpPairList().at(j).second > d_magThresh*thisLineOffInt)
                                    {
                                        resultList[i] = false;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                resultList[i] = false;
                            }
                        }
                    }

                    //magnet will always be disabled for final scan
                }//end if magnet

                //add labels if this is a categorizing test
                if(d_testList.at(d_status.currentTestIndex).categorize)
                {
                    QStringList cats;
                    if(!d_status.category.isEmpty())
                        cats = d_status.category.split(QString("/"));
                    for(int i=0; i<numLines; i++)
                    {
                        while(i >= cats.size())
                            cats.append(QString(""));
                        if(!cats.at(i).isEmpty())
                            cats[i].append(QString(";"));
                        cats[i].append(d_status.currentTestKey);
                        if(resultList.first())
                            cats[i].append(QString(":1"));
                        else
                            cats[i].append(QString(":0"));
                    }

                    d_status.category = cats.join(QChar('/'));
                }
            }
        } // end DC or magnetic
        else
        {
            /// find best scan, build template from that scan number
            /// if multiple lines, use closest to center to determine scan settings to use (this will be the first in the list)
            /// if this test is part of the category label, add it to the category (do this for each line)
            int bestScan = -1;
            QList<QPair<double,QVariant>> bestValues;
            for(int i=0; i<numLines; i++)
                bestValues.append(qMakePair(0.0,QVariant()));

            for(int k=0; k<tests.size(); k++)
            {
			 TestResult tr = tests.at(k);
                if(tr.extraAttn < d_status.currentExtraAttn)
                    continue;

			 bool isBetter = false;
                //only one "line", use ftMax
                if(tr.result.type() == FitResult::NoFitting)
                {
                    if(tr.ftMax > bestValues.first().first)
                    {
					isBetter = true;
                        bestValues[0].first = tr.ftMax;
                        bestValues[0].second = tr.value;
                        bestScan = tr.scanNum;
                    }
                }
                else
                {
                    //try to find each frequency in fit result. If better, add it to bestvalues
                    for(int i=0; i<numLines; i++)
                    {
				    double thisLineFreq = d_status.frequencies.at(i);
                        for(int j=0; j<tr.result.freqAmpPairList().size(); j++)
                        {
                            if(qAbs(thisLineFreq-tr.result.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff && tr.result.freqAmpPairList().at(j).second > bestValues.at(i).first)
                            {
                                bestValues[i].first = tr.result.freqAmpPairList().at(j).second;
                                bestValues[i].second = tr.value;
						  if(i==0)
						  {
							  isBetter = true;
							  bestScan = tr.scanNum;
						  }
                                break;
                            }
                        }

                    }
                }

			 if(isBetter && tr.key == QString("u"))
             {
                 d_status.bestDipoleResult.extraAttn = tr.extraAttn;
                 d_status.bestDipoleResult.ftMax = tr.ftMax;
                 d_status.bestDipoleResult.key = tr.key;
                 d_status.bestDipoleResult.result = tr.result;
                 d_status.bestDipoleResult.scanNum = tr.scanNum;
                 d_status.bestDipoleResult.value = tr.value;
             }
            } // end iteration over test results

            //at this point, bestResults contains the best values for each frequency.
            //create labels if applicable and set scan
            if(d_testList.at(d_status.currentTestIndex).categorize)
            {
                QStringList cats;
                if(!d_status.category.isEmpty())
                    cats = d_status.category.split(QString("/"));
                for(int i=0; i<numLines; i++)
                {
                    while(i >= cats.size())
                        cats.append(QString(""));
                    if(!cats.at(i).isEmpty())
                        cats[i].append(QString(";"));
                    cats[i].append(QString("%1:%2").arg(d_status.currentTestKey).arg(bestValues.at(i).second.toString()));
                }

                d_status.category = cats.join(QChar('/'));
            }

            if(bestScan > 0)
                d_status.scanTemplate = Scan::settingsFromPrevious(bestScan);

        } //end condition not dc or m test
    }//end tests empty and frequencies empty test

    d_status.scanTemplate.setMagnet(false);
    d_status.scanTemplate.setDipoleMoment(0.0);
}
