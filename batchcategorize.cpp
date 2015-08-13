#include "batchcategorize.h"

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

	//need to estimate total shots
	//will need to be adjusted if a scan is saturated or no line is detected
	d_totalShots = 0;
	for(int i=0; i<d_templateList.size(); i++)
	{
		if(d_templateList.at(i).second)
			d_totalShots += d_templateList.at(i).first.targetShots();
		else
		{
			//total number of shots depends on which tests are enabled
			int numTests = 0;
			for(int j=0; j<d_testList.size(); j++)
				numTests += d_testList.at(j).valueList.size();
			d_totalShots += numTests*d_templateList.at(i).first.targetShots();
		}
	}

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
            }
            else if(d_status.frequencies.isEmpty())
            {
                //check to see if lines are present
                //if fitter is not being used, then we just say 1 line is here and go with the FT Max
                if(d_fitter->type() == FitResult::NoFitting)
                    d_status.frequencies.append(s.ftFreq());
                else
                {
                    for(int i=0; i<res.freqAmpPairList().size(); i++)
                        d_status.frequencies.append(res.freqAmpPairList().at(i).first);
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
        //exception: magnet always stays off after (though, since it's usually last test, shoudln't matter)
        auto tests = d_status.resultMap.values(d_status.currentTestKey);
        if(!tests.isEmpty())
        {
            ///TODO: IMPLEMENT - find best scan, build template from that scan number
            /// if multiple lines, use strongest to determine scan settings to use
            /// if this test is part of the category label, add it to the category (do this for each line)
        }
        d_status.scanTemplate.setMagnet(false);
        d_status.scanTemplate.setDipoleMoment(0.0);
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
