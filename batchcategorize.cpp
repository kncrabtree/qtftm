#include "batchcategorize.h"

BatchCategorize::BatchCategorize(QList<QPair<Scan, bool> > scanList, QList<CategoryTest> testList, AbstractFitter *ftr) :
	BatchManager(QtFTM::Categorize,false,ftr), d_templateList(scanList), d_testList(testList), d_currentScanIndex(0),
	d_currentTestIndex(0), d_currentValueIndex(0), d_thisScanIsRef(false)
{
	d_prettyName = QString("Category Test");

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

		d_currentScanIndex++;
	}
	else
	{
		double num = (double)s.number();

		auto p = d_fitter->doStandardFT(s.fid());
		QVector<QPointF> ft = p.first;
		double max = p.second;

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
			//use results to determine what scan to do next

			//0.) If this is the reference scan, assign category and move on to next scan in list
			//1.) If this is the first test for this template, check for saturation and adjust attn if necessary
			//1a.) Continue checking for saturation until not saturated; first test complete
			//2.) If no line detected, continue with dipole tests (if applicable) or move to next template
			//3.) Proceed through tests.
			//4.) When final test is complete, choose best settings and re-scan. This is the reference scan.

		}
	}

	QtFTM::BatchPlotMetaData md(d_batchType,s.number(),mdMin,mdMax,d_thisScanIsCal,badTune,labelText);
	md.isRef = d_thisScanIsRef;
	QList<QVector<QPointF>> out;
	if(d_thisScanIsCal)
		out.append(d_calData);
	else
		out.append(d_scanData);

	emit plotData(md,out);
}

void BatchCategorize::processScan(Scan s)
{
}

Scan BatchCategorize::prepareNextScan()
{
}

bool BatchCategorize::isBatchComplete()
{
}
