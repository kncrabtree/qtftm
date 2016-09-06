#include "dopplerpairfitter.h"
#include <gsl/gsl_fit.h>

#include "analysis.h"

DopplerPairFitter::DopplerPairFitter(QObject *parent) :
    AbstractFitter(FitResult::DopplerPair, parent)
{
}

FitResult DopplerPairFitter::doFit(const Scan s)
{
    Fid fid = ftw.filterFid(s.fid());
    FitResult out(FitResult::DopplerPair,FitResult::Invalid);
	out.setProbeFreq(fid.probeFreq());
	out.setDelay(ftw.delay());
	out.setHpf(ftw.hpf());
	out.setExp(ftw.exp());
    out.setRdc(ftw.removeDC());
    out.setZpf(ftw.autoPad());
    out.setUseWindow(ftw.isUseWindow());
	out.setTemperature(d_temperature);
	out.setBufferGas(d_bufferGas);
    FitResult::LineShape lsf = FitResult::Lorentzian;
    if(ftw.isUseWindow())
        lsf = FitResult::Gaussian;
    out.setLineShape(lsf);



    out.appendToLog(QString("Beginning autofit of scan %1.").arg(s.number()));

	if(fid.size() < 10)
	{
        out.appendToLog(QString("The FID is less than 10 points in length. Fitting aborted."));
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	//if the FID is saturated, it's not worth wasting any time fitting it because the parameters will be wrong
    if(isFidSaturated(fid))
	{
        out.appendToLog(QString("The FID is saturated. Fitting aborted."));
		out.setCategory(FitResult::Saturated);
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	//remove DC from fid and FT it
    out.appendToLog(QString("Computing FT."));
    Fid f = Analysis::removeDC(s.fid());
    QVector<QPointF> ftBl = ftw.doFT_pad(f,true);
	QVector<QPointF> ftPad = ftw.doFT_pad(f,true);

	if(ftBl.size() < 10 || ftPad.size() < 10)
	{
        out.appendToLog(QString("The size of the calculated FT is less than 10 points. Fitting aborted."));
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	//do peakfinding: try to estimate baseline and noise level
    out.appendToLog(QString("Estimating baseline..."));
	QList<double> blData = Analysis::estimateBaseline(ftBl);
	if(blData.size() < 4)
	{
        out.appendToLog(QString("Baseline estimation failed! Fitting aborted."));
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

    out.appendToLog(QString("Baseline estimation successful! Approximate parameters:"));
    out.appendToLog(QString("y0\t%1").arg(QString::number(blData.at(0),'f',6)));
    out.appendToLog(QString("m\t%1").arg(QString::number(blData.at(1),'f',6)));
    out.appendToLog(QString("sigma0\t%1").arg(QString::number(blData.at(2),'f',6)));
    out.appendToLog(QString("sigmam\t%1").arg(QString::number(blData.at(3),'f',6)));

	//remove baseline from ft and find peaks
    out.appendToLog(QString("Subtracting baseline and finding peaks."));
	ftBl = Analysis::removeBaseline(ftBl,blData.at(0),blData.at(1));
    double snr = 6.0;
//    if(ftw.isUseWindow())
//        snr = 6.0; //larger SNR threshold for BH-window to reduce false positives
    QList<QPair<QPointF,double> > peakList = findPeaks(ftBl,blData.at(2),blData.at(3),snr);
	out.setBaselineY0Slope(blData.at(0),blData.at(1));
	if(peakList.size() == 0)
	{
        out.appendToLog(QString("No peaks detected. Fitting to line."));
        out = Analysis::fitLine(out,ftPad,fid.probeFreq());
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

    out.appendToLog(QString("Found %1 peaks:").arg(peakList.size()));
    out.appendToLog(QString("Num\tFreq\tA\tSNR"));
	for(int i=0; i<peakList.size(); i++)
	{
            out.appendToLog(QString("%1\t%2\t%3\t%4").arg(i).arg(QString::number(peakList.at(i).first.x(),'f',3))
                     .arg(QString::number(peakList.at(i).first.y(),'f',2)).arg(QString::number(peakList.at(i).second,'f',2)));
	}


	double ftSpacing = ftBl.at(1).x() - ftBl.at(0).x();
	double splitting = Analysis::estimateSplitting(d_bufferGas,d_temperature,fid.probeFreq());
    out.appendToLog(QString("Estimated Doppler splitting: %1 MHz.").arg(QString::number(splitting,'f',6)));
	double width = Analysis::estimateDopplerLinewidth(d_bufferGas,fid.probeFreq());
    out.appendToLog(QString("Estimated linewidth: %1 MHz.").arg(QString::number(width,'f',6)));
    out.appendToLog(QString("Looking for possible Doppler pairs..."));
    QList<FitResult::DopplerPairParameters> dpParams = Analysis::estimateDopplerCenters(peakList,splitting,ftSpacing);

    out.appendToLog(QString("Found %1 possible pairs.").arg(dpParams.size()));

	//look for strong peaks whose other doppler component might fall outside window
	//these can mess up the fit for smaller features in the center
    out.appendToLog(QString("Looking for strong unpaired peaks near edge of FT."));
	QList<QPointF> singlePeaks;
	singlePeaks.reserve(peakList.size());
	for(int i=0; i<peakList.size(); i++)
	{
        //if SNR is less than 20, it probably won't mess up the fit if it's left out
        if(peakList.at(i).second < 20.0)
			continue;

		//make sure its doppler partner would be outside range
		double freq = peakList.at(i).first.x();
		if((freq - splitting) > 0.01 && (freq + splitting) < 0.99 )
			continue;

		//make sure this isn't part of a doppler pair we're already fitting
		bool peakIsDoppler = false;
		for(int j=0; j<dpParams.size(); j++)
		{
			if(fabs(freq - (dpParams.at(j).centerFreq-splitting/2.0)) < 0.5*ftSpacing
					|| fabs(freq - (dpParams.at(j).centerFreq+splitting/2.0)) < 0.5*ftSpacing)
			{
				peakIsDoppler = true;
				break;
			}
		}
		if(peakIsDoppler)
			continue;

		//if we reach this point, we have to fit to a mix of pairs and single peaks
		//put this peak in a list
		singlePeaks.append(peakList.at(i).first);
	}

    out.appendToLog(QString("Found %1 strong single peaks.").arg(singlePeaks.size()));

	if(singlePeaks.size() == 0 && dpParams.size() == 0)
	{
        out.appendToLog(QString("No valid Doppler pairs or strong single peaks found. Fitting to line..."));
        out = Analysis::fitLine(out,ftPad,fid.probeFreq());
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	if(dpParams.size()>10)
        out.appendToLog(QString("Truncating Doppler pair list to the strongest 10."));

	while(dpParams.size()>10)
		dpParams.removeLast();

    if(singlePeaks.size()>10)
        out.appendToLog(QString("Truncating single peak list to the strongest 10."));

	while(singlePeaks.size()>10)
		singlePeaks.removeLast();


	//at this point, there are 3 possibilities:
	//Pairs only, Mixed pairs and singles, and singles only
	//during fitting, we may transition from mixed->pairs only, or from mixed->singles only
	bool done = false;
	int iterations = 50;

    if(lsf == FitResult::Lorentzian)
        out.appendToLog(QString("Using Lorentzian lineshape."));
    else if(lsf == FitResult::Gaussian)
        out.appendToLog(QString("Using Gaussian lineshape."));

    QList<double> commonParams;
    commonParams << blData.at(0) << blData.at(1) << splitting << width;

	while(!done)
	{
		if(singlePeaks.size() == 0 && dpParams.size() == 0)
		{
            out.appendToLog(QString("No peaks remain. Fitting to line..."));
            out = Analysis::fitLine(out,ftPad,fid.probeFreq());
			break;
		}

        out = dopplerFit(ftPad,out,commonParams,dpParams,singlePeaks,iterations);

        if(out.category() == FitResult::Fail)
        {
            if(out.iterations() == iterations && iterations < 1000)
            {
                out.appendToLog(QString("Fit did not converge after %1 iterations. Increasing to %2.").arg(iterations).arg(iterations*2));
                iterations *= 2;
                continue;
            }

            iterations = 50;
            if(!singlePeaks.isEmpty())
            {
                singlePeaks.removeLast();
                continue;
            }
            dpParams.removeLast();
            continue;
        }

		bool spurious = false;

        //check single peaks
        if(out.freqAmpSingleList().size()>0)
        {
            //amplitudes
            out.appendToLog(QString("Checking single peak parameters...."));
            for(int i=out.freqAmpSingleList().size()-1; i>=0; i--)
            {
                if(out.freqAmpSingleList().at(i).second <  5.0*(blData.at(2) + blData.at(3)*(out.freqAmpSingleList().at(i).first-out.probeFreq()))
                        || out.freqAmpSingleList().at(i).second> 100.0
                        || out.freqAmpSingleList().at(i).second < 0.0)
                {
                    out.appendToLog(QString("Removing single peak %1 because its amplitude (%2) is bad.").arg(i)
                               .arg(QString::number(out.freqAmpSingleList().at(i).second,'f',2)));
                    singlePeaks.removeAt(i);
                    spurious = true;
                    break;
                }
            }

            if(spurious)
            {
                iterations = 50;
                continue;
            }

            //check uncertainties
            for(int i=out.freqAmpSingleList().size()-1; i>=0; i--)
            {
                if(out.freqAmpSingleList().at(i).second < out.freqAmpSingleUncList().at(i).second)
                {
                    out.appendToLog(QString("Removing single peak %1 because its amplitude (%2) is less than its uncertainty (%3)").arg(i)
                               .arg(QString::number(out.freqAmpSingleList().at(i).second,'f',2))
                               .arg(QString::number(out.freqAmpSingleUncList().at(i).second,'f',2)));
                    singlePeaks.removeAt(i);
                    spurious = true;
                    break;
                }
            }

            if(spurious)
            {
                iterations = 50;
                continue;
            }
        }

		if(out.freqAmpPairList().size()>0)
            out.appendToLog(QString("Checking Doppler pair parameters..."));
		for(int i=out.freqAmpPairList().size()-1; i>=0; i--)
		{
			if(out.freqAmpPairList().at(i).second < 4.0*(blData.at(2) + blData.at(3)*(out.freqAmpPairList().at(i).first - out.probeFreq()))
					|| out.freqAmpPairList().at(i).second > 100.0
					|| out.freqAmpPairList().at(i).second < 0.0)
			{
                out.appendToLog(QString("Removing Doppler pair %1 because its amplitude is bad (%2).").arg(i)
						 .arg(QString::number(out.freqAmpPairList().at(i).second,'f',2)));
				dpParams.removeAt(i);
				spurious = true;
				break;
			}

			if(out.allFitParams().at(5+3*i) < Analysis::alphaTolerance || out.allFitParams().at(5+3*i) > (1.0-Analysis::alphaTolerance))
			{
                out.appendToLog(QString("Removing Doppler pair %1 because its alpha is bad (%2).").arg(i)
						 .arg(QString::number(out.allFitParams().at(5+3*i),'f',2)));
				dpParams.removeAt(i);
				spurious = true;
				break;
			}
		}

		if(spurious)
		{
			iterations = 50;
			continue;
		}

		if(out.freqAmpPairList().size() > 1)
		{
            out.appendToLog(QString("Checking Doppler pair uncertainties because the number of pairs is more than 1."));
			for(int i=out.freqAmpPairList().size()-1; i>=0; i--)
			{
				if(2.0*out.freqAmpPairList().at(i).second < out.freqAmpPairUncList().at(i).second)
				{
                    out.appendToLog(QString("Removing weakest Doppler pair because the amplitude of peak %1 (%2) is less than half its uncertainty (%3).").arg(i)
							 .arg(QString::number(out.freqAmpPairList().at(i).second,'f',2))
							.arg(QString::number(out.freqAmpPairUncList().at(i).second,'f',2)));
                    dpParams.removeLast();
					spurious = true;
					break;
				}

				if(out.allFitParams().at(5+3*i) < out.allFitUncertainties().at(5+3*i))
				{
                    out.appendToLog(QString("Removing weakest Doppler pair the alpha of peak %1 (%2) is less than its uncertainty (%3).").arg(i)
							 .arg(QString::number(out.allFitParams().at(5+3*i),'f',2))
							.arg(QString::number(out.allFitUncertainties().at(5+3*i),'f',2)));
                    dpParams.removeLast();
					spurious = true;
					break;
				}
			}
		}

		if(spurious)
		{
			iterations = 50;
			continue;
		}

		if(dpParams.size() > 0) //check width and splitting
		{
			//if something looks wrong, remove weakest peak and try again
			if(out.allFitParams().at(3) < 0.5*ftSpacing)
			{
                out.appendToLog(QString("Fit gave too small a linewidth (%1). Removing weakest pair.")
						 .arg(QString::number(out.allFitParams().at(3),'f',5)));
				dpParams.removeLast();
				iterations = 50;
				continue;
			}

			if(fabs(out.allFitParams().at(2)-splitting) > 0.5*splitting)
			{
                out.appendToLog(QString("Fit gave a splitting more than 50% away from guess (%1). Removing weakest pair.")
						 .arg(QString::number(out.allFitParams().at(2),'f',5)));
				dpParams.removeLast();
				iterations = 50;
				continue;
			}

			if(dpParams.size()>1)
			{
				if(out.allFitParams().at(3) < out.allFitUncertainties().at(3))
				{
                    out.appendToLog(QString("Fit linewidth (%1) is less than uncertainty (%2). Removing weakest pair.")
							 .arg(QString::number(out.allFitParams().at(3),'f',5))
							 .arg(QString::number(out.allFitUncertainties().at(3),'f',5)));
					dpParams.removeLast();
					iterations = 50;
					continue;
				}

				if(out.allFitParams().at(2) < out.allFitUncertainties().at(2))
				{
                    out.appendToLog(QString("Fit splitting (%1) is less than uncertainty (%2). Removing weakest pair.")
							 .arg(QString::number(out.allFitParams().at(2),'f',5))
							 .arg(QString::number(out.allFitUncertainties().at(2),'f',5)));
					dpParams.removeLast();
					iterations = 50;
					continue;
				}
			}
		}
		else //at this point, we must be fitting single peaks alone
		{
			//check amplitudes
			for(int i=out.freqAmpSingleList().size()-1; i>=0; i--)
			{
				if(out.freqAmpSingleList().at(i).second
						< 5.0*(blData.at(2) + blData.at(3)*(out.freqAmpSingleList().at(i).first-out.probeFreq()))
						|| out.freqAmpSingleList().at(i).second> 100.0
						|| out.freqAmpSingleList().at(i).second < 0.0)
				{
                    out.appendToLog(QString("Removing single peak %1 because its amplitude (%2) is less than its uncertainty (%3)").arg(i)
							 .arg(QString::number(out.freqAmpSingleList().at(i).second,'f',2))
							.arg(QString::number(out.freqAmpSingleUncList().at(i).second,'f',2)));
					singlePeaks.removeAt(i);
					spurious = true;
					break;
				}
			}

			if(spurious)
			{
				iterations = 50;
				continue;
			}

			//check uncertainties
			for(int i=out.freqAmpSingleList().size()-1; i>=0; i--)
			{
				if(out.freqAmpSingleList().at(i).second < 2.0*out.freqAmpSingleUncList().at(i).second)
				{
                    out.appendToLog(QString("Removing single peak %1 because its amplitude (%2) is less than its uncertainty (%3)").arg(i)
							 .arg(QString::number(out.freqAmpSingleList().at(i).second,'f',2))
							.arg(QString::number(out.freqAmpSingleUncList().at(i).second,'f',2)));
					singlePeaks.removeAt(i);
					spurious = true;
					break;
				}
			}

			if(spurious)
			{
				iterations = 50;
				continue;
			}


			//check width
			if(out.allFitParams().at(2) < 0.5*ftSpacing || out.allFitParams().at(2) < out.allFitUncertainties().at(2))
			{
                out.appendToLog(QString("Fit linewidth (%1) is less than uncertainty (%2). Removing weakest peak.")
						 .arg(QString::number(out.allFitParams().at(2),'f',5))
						 .arg(QString::number(out.allFitUncertainties().at(2),'f',5)));
				singlePeaks.removeLast();
				iterations = 50;
				continue;
			}
		}

		if(!spurious)
		{
			if(out.status() != GSL_SUCCESS)
			{
				iterations = iterations*2;
			}

			if(out.status() == GSL_SUCCESS || out.status() == GSL_ENOPROG || iterations > 1000)
				done = true;
		}
	}

	out.save(s.number());
	emit fitComplete(out);
	return out;
}
