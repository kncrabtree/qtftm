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
            d_testList.append(testList.at(i));
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

BatchCategorize::~BatchCategorize()
{

}

void BatchCategorize::writeReport()
{
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

	double mdMin = s.number();
	double mdMax = s.number();
	bool badTune = (s.tuningVoltage() <= 0);
	QString labelText;
    bool isRef = false;

	if(d_thisScanIsCal)
	{
		double intensity = 0.0;
		if(res.freqAmpPairList().isEmpty())
			intensity = d_fitter->doStandardFT(s.fid()).second;
		else
		{
			//record strongest peak intensity
			for(int i=0; i<res.freqAmpPairList().size(); i++)
				intensity = qMax(intensity,res.freqAmpPairList().at(i).second);
		}

		d_calData.append(QPointF(static_cast<double>(s.number()),intensity));
		labelText = QString("CAL");

        d_status.advance();
        emit advanced();
	}
	else
	{
		double num = (double)s.number();

		auto p = d_fitter->doStandardFT(s.fid());
		QVector<QPointF> ft = p.first;
		double max = p.second;
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

		if(d_loading)
		{
			//build label from info in file
			//create a list of labels in loading constructor
		}
		else
		{
            labelText.append(QString("%1/%2\n").arg(s.ftFreq(),0,'f',3).arg(s.attenuation()));
            d_status.scanTemplate.setAttenuation(s.attenuation());
            //build scan result; make initial settings.
            //some will be overwritten later as the scan is processed
            d_status.scansTaken++;
            ScanResult sr;
            sr.scanNum = s.number();
            sr.attenuation = s.attenuation();
            sr.testKey = d_status.currentTestKey;
            sr.testValue = d_status.currentTestValue;
            sr.frequencies = d_status.frequencies;

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
                sr.testKey = QString("sat");
                sr.testValue = d_status.currentExtraAttn;
                if(d_status.currentExtraAttn + 10 >= d_maxAttn)
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
                    d_status.scanTemplate.setAttenuation(d_status.currentExtraAttn);
                    d_status.scanTemplate.setDipoleMoment(0.0);
                    labelText.append(QString("SAT"));
                }

                if(d_status.currentTestKey == QString("u"))
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
            }
            else if(d_status.currentTestKey == QString("final"))
            {
                //categorize
                sr.testValue = d_status.category;
                labelText.append(QString("FINAL\n%1").arg(d_status.category));
                isRef = true;
                d_status.advance();
                emit advanced();
            }
            else if(d_status.frequencies.isEmpty())
            {
                //check to see if lines are present
                //if fitter is not being used, then we just say 1 line is here and go with the FT Max
                if(d_fitter->type() == FitResult::NoFitting)
                    d_status.frequencies.append(s.ftFreq());
                else
                {
                    //the line closest to the ft frequency comes first
                    for(int i=0; i<res.freqAmpPairList().size(); i++)
                    {
                        if(d_status.frequencies.isEmpty())
                            d_status.frequencies.append(res.freqAmpPairList().at(i).first);
                        else if(qAbs(d_status.frequencies.first()-s.ftFreq()) < qAbs(res.freqAmpPairList().at(i).first-s.ftFreq()))
                            d_status.frequencies.prepend(res.freqAmpPairList().at(i).first);
                        else
                            d_status.frequencies.append(res.freqAmpPairList().at(i).first);
                    }
                }

                sr.frequencies = d_status.frequencies;

                //chick to see if the frequency list is still empty
                //if we're doing a dipole test, we can continue
                //otherwise, or if the dipole test is done, this is a non-detection
                if(d_status.frequencies.isEmpty())
                {
                    if((d_status.currentTestKey == QString("u") && d_status.currentValueIndex + 1 >= d_testList.at(d_status.currentTestIndex).valueList.size()) || d_status.currentTestKey != QString("u"))
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
                else
                {
                    labelText.append(QString("%1:%2").arg(d_status.currentTestKey).arg(d_status.currentTestValue.toString()));
                    d_status.resultMap.insertMulti(d_status.currentTestKey,tr);
                    setNextTest();
                }
            }
            else
            {
                //consider possiblity of adding new lines if this is a dipole test?
                labelText.append(QString("%1:%2").arg(d_status.currentTestKey).arg(d_status.currentTestValue.toString()));
                d_status.resultMap.insertMulti(d_status.currentTestKey,tr);
                setNextTest();
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
        d_status.currentTestKey = QString("final");
        return true;
    }

    d_status.scanTemplate.setDipoleMoment(0.0);
    d_status.scanTemplate.setMagnet(false);
    configureScanTemplate();

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
        if(d_status.currentTestKey == QString("dc") || d_status.currentTestKey == QString("mag"))
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
                            tr.ftMax = vtr.ftMax;
                            tr.result = vtr.result;

                        } // end if voltages match
                    } // end loop over voltage tests
                } // end if voltage tests
                else if(d_status.resultMap.contains(QString("u"))) //otherwise, there must be a dipole test, because there is only 1 dc/mag test if a previous test has been done
                {
                    auto uTests = d_status.resultMap.values(QString("u"));
                    for(int i=0; i<uTests.size(); i++)
                    {
                        const TestResult &utr = uTests.at(i);
                        //use "best dipole" from current status
                        if(qAbs(utr.value.toDouble() - d_status.bestDipole) < d_lineMatchMaxDiff)
                        {
                            //the value from the dipole test is opposite the current value (because magnet/discharge was toggled)
                            tr.extraAttn = utr.extraAttn;
                            tr.value = !tests.first().value.toBool();
                            tr.ftMax = utr.ftMax;
                            tr.result = utr.result;
                        } // end if dipoles match
                    } // end loop over dipole tests
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
                    }
                    else
                    {
                        //if the "off" test contains a line at the given frequency, then it's NOT a DC line
                        for(int i=0; i<numLines; i++)
                        {
                            double thisLineFreq = d_status.frequencies.at(i);
                            resultList[i] = true;
                            for(int j=0; j<offTest.result.freqAmpPairList().size(); j++)
                            {
                                if(qAbs(thisLineFreq-offTest.result.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff)
                                {
                                    resultList[i] = false;
                                    break;
                                }
                            }
                        }
                    }

                    //set next scan to DC on or off, depending on line closest to center (first in resultList)
                    PulseGenConfig pc = d_status.scanTemplate.pulseConfiguration();
                    pc.setDcEnabled(resultList.first());
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
                            resultList[i] = false;
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
                                    if(qAbs(thisLineFreq-onTest.result.freqAmpPairList().at(j).first) < d_lineMatchMaxDiff && onTest.result.freqAmpPairList().at(j).second < d_magThresh*thisLineOffInt)
                                    {
                                        resultList[i] = true;
                                        break;
                                    }
                                }
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
                const TestResult &tr = tests.at(k);
                if(tr.extraAttn < d_status.currentExtraAttn)
                    continue;

                //only one "line", use ftMax
                if(tr.result.type() == FitResult::NoFitting)
                {
                    if(tr.ftMax > bestValues.first().first)
                    {
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
                                bestScan = tr.scanNum;
                                break;
                            }
                        }
                    }
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
