#include "dopplerpairfitter.h"
#include <gsl/gsl_fit.h>

DopplerPairFitter::DopplerPairFitter(QObject *parent) :
    AbstractFitter(FitResult::DopplerPair, parent)
{
}

FitResult DopplerPairFitter::doFit(const Scan s)
{
	QString log;

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



	log.append(QString("Beginning autofit of scan %1.\n").arg(s.number()));

	if(fid.size() < 10)
	{
		log.append(QString("The FID is less than 10 points in length. Fitting aborted.\n"));
		out.appendToLog(log);
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	//if the FID is saturated, it's not worth wasting any time fitting it because the parameters will be wrong
	if(Analysis::isFidSaturated(fid))
	{
		log.append(QString("The FID is saturated. Fitting aborted.\n"));
		out.setCategory(FitResult::Saturated);
		out.appendToLog(log);
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	//remove DC from fid and FT it
	log.append(QString("Computing FTs with and without zero padding.\n"));
    Fid f = Analysis::removeDC(s.fid());
    QVector<QPointF> ftBl = ftw.doFT_pad(f,true);
	QVector<QPointF> ftPad = ftw.doFT_pad(f,true);

	if(ftBl.size() < 10 || ftPad.size() < 10)
	{
		log.append(QString("The size of the calculated FT is less than 10 points. Fitting aborted.\n"));
		out.appendToLog(log);
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	//do peakfinding: try to estimate baseline and noise level
	log.append(QString("Estimating baseline...\n"));
	QList<double> blData = Analysis::estimateBaseline(ftBl);
	if(blData.size() < 4)
	{
		log.append(QString("Baseline estimation failed! Fitting aborted.\n"));
		out.appendToLog(log);
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	log.append(QString("Baseline estimation successful! Approximate parameters:\n"));
	log.append(QString("y0\t%1\n").arg(QString::number(blData.at(0),'f',6)));
	log.append(QString("m\t%1\n").arg(QString::number(blData.at(1),'f',6)));
	log.append(QString("sigma0\t%1\n").arg(QString::number(blData.at(2),'f',6)));
	log.append(QString("sigmam\t%1\n").arg(QString::number(blData.at(3),'f',6)));

	//remove baseline from ft and find peaks
	log.append(QString("Subtracting baseline and finding peaks.\n"));
	ftBl = Analysis::removeBaseline(ftBl,blData.at(0),blData.at(1));
    double snr = 3.0;
    if(ftw.isUseWindow())
        snr = 6.0; //larger SNR threshold for BH-window to reduce false positives
    QList<QPair<QPointF,double> > peakList = Analysis::findPeaks(ftBl,blData.at(2),blData.at(3),snr);
	out.setBaselineY0Slope(blData.at(0),blData.at(1));
	if(peakList.size() == 0)
	{
		log.append(QString("No peaks detected. Fitting to line.\n"));
		out = Analysis::fitLine(ftPad,fid.probeFreq());
		out.setDelay(ftw.delay());
		out.setHpf(ftw.hpf());
		out.setExp(ftw.exp());
        out.setRdc(ftw.removeDC());
        out.setZpf(ftw.autoPad());
        out.setUseWindow(ftw.isUseWindow());
		out.setTemperature(d_temperature);
		out.setBufferGas(d_bufferGas);
		out.appendToLog(log);
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	log.append(QString("Found %1 peaks:\n").arg(peakList.size()));
	log.append(QString("Num\tFreq\tA\tSNR\n"));
	for(int i=0; i<peakList.size(); i++)
	{
			log.append(QString("%1\t%2\t%3\t%4\n").arg(i).arg(QString::number(peakList.at(i).first.x(),'f',3))
                     .arg(QString::number(peakList.at(i).first.y(),'f',2)).arg(QString::number(peakList.at(i).second,'f',2)));
	}


	double ftSpacing = ftBl.at(1).x() - ftBl.at(0).x();
	double splitting = Analysis::estimateSplitting(d_bufferGas,d_temperature,fid.probeFreq());
	log.append(QString("Estimated Doppler splitting: %1 MHz.\n").arg(QString::number(splitting,'f',6)));
	double width = Analysis::estimateDopplerLinewidth(d_bufferGas,fid.probeFreq());
	log.append(QString("Estimated linewidth: %1 MHz.\n").arg(QString::number(width,'f',6)));
	log.append(QString("Looking for possible Doppler pairs...\n"));
	QList<Analysis::DopplerPairParameters> dpParams = Analysis::estimateDopplerCenters(peakList,splitting,ftSpacing);

	log.append(QString("Found %1 possible pairs.\n").arg(dpParams.size()));

	//look for strong peaks whose other doppler component might fall outside window
	//these can mess up the fit for smaller features in the center
	log.append(QString("Looking for strong unpaired peaks near edge of FT.\n"));
	QList<QPointF> singlePeaks;
	singlePeaks.reserve(peakList.size());
	for(int i=0; i<peakList.size(); i++)
	{
		//if SNR is less than 5, it probably won't mess up the fit if it's left out
		if(peakList.at(i).second < 5.0)
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

	log.append(QString("Found %1 strong single peaks.\n").arg(singlePeaks.size()));

	if(singlePeaks.size() == 0 && dpParams.size() == 0)
	{
		log.append(QString("No valid Doppler pairs or strong single peaks found. Fitting to line...\n"));
		out = Analysis::fitLine(ftPad,fid.probeFreq());
		out.setDelay(ftw.delay());
		out.setHpf(ftw.hpf());
		out.setExp(ftw.exp());
        out.setRdc(ftw.removeDC());
        out.setZpf(ftw.autoPad());
        out.setUseWindow(ftw.isUseWindow());
		out.setTemperature(d_temperature);
		out.setBufferGas(d_bufferGas);
		out.appendToLog(log);
		out.save(s.number());
		emit fitComplete(out);
		return out;
	}

	if(dpParams.size()>10)
		log.append(QString("Truncating Doppler pair list to the strongest 10.\n"));

	while(dpParams.size()>10)
		dpParams.removeLast();

    if(singlePeaks.size()>10)
		log.append(QString("Truncating single peak list to the strongest 10.\n"));

	while(singlePeaks.size()>10)
		singlePeaks.removeLast();


	//at this point, there are 3 possibilities:
	//Pairs only, Mixed pairs and singles, and singles only
	//during fitting, we may transition from mixed->pairs only, or from mixed->singles only
	bool done = false;
	int iterations = 50;
    FitResult::LineShape lsf = FitResult::Lorentzian;
    if(ftw.isUseWindow())
        lsf = FitResult::Gaussian;

    if(lsf == FitResult::Lorentzian)
        log.append(QString("Using Lorentzian lineshape.\n"));
    else if(lsf == FitResult::Gaussian)
        log.append(QString("Using Gaussian lineshape.\n"));

	while(!done)
	{
		if(singlePeaks.size() == 0 && dpParams.size() == 0)
		{
			log.append(QString("No peaks remain. Fitting to line...\n"));
			out = Analysis::fitLine(ftPad,fid.probeFreq());
			break;
		}

		if(singlePeaks.size() == 0)
		{
			log.append(QString("Fitting to %1 Doppler pairs (%2 max iterations)...\n").arg(dpParams.size()).arg(iterations));
            out = Analysis::dopplerPairFit(gsl_multifit_fdfsolver_lmsder,lsf,ftPad,fid.probeFreq(),blData.at(0),blData.at(1),splitting,width,dpParams,iterations);
		}
		else if(dpParams.size() == 0)
		{
			log.append(QString("Fitting to %1 single peaks (%2 max iterations)...\n").arg(singlePeaks.size()).arg(iterations));
            out = Analysis::singleFit(gsl_multifit_fdfsolver_lmsder,lsf,ftPad,fid.probeFreq(),blData.at(0),blData.at(1),width,singlePeaks,iterations);
		}
		else
		{
			log.append(QString("Fitting to %1 Doppler pairs and %2 single peaks (%3 max iterations)...\n").arg(dpParams.size())
					 .arg(singlePeaks.size()).arg(iterations));
            out = Analysis::dopplerMixedFit(gsl_multifit_fdfsolver_lmsder,lsf,ftPad,fid.probeFreq(),blData.at(0),blData.at(1),splitting,width,dpParams,singlePeaks,iterations);
		}

		bool spurious = false;
		if(out.freqAmpPairList().size()>0)
			log.append(QString("Checking Doppler pair parameters...\n"));
		for(int i=out.freqAmpPairList().size()-1; i>=0; i--)
		{
			if(out.freqAmpPairList().at(i).second < 4.0*(blData.at(2) + blData.at(3)*(out.freqAmpPairList().at(i).first - out.probeFreq()))
					|| out.freqAmpPairList().at(i).second > 100.0
					|| out.freqAmpPairList().at(i).second < 0.0)
			{
				log.append(QString("Removing Doppler pair %1 because its amplitude is bad (%2).\n").arg(i)
						 .arg(QString::number(out.freqAmpPairList().at(i).second,'f',2)));
				dpParams.removeAt(i);
				spurious = true;
				break;
			}

			if(out.allFitParams().at(5+3*i) < Analysis::alphaTolerance || out.allFitParams().at(5+3*i) > (1.0-Analysis::alphaTolerance))
			{
				log.append(QString("Removing Doppler pair %1 because its alpha is bad (%2).\n").arg(i)
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
			log.append(QString("Checking Doppler pair uncertainties because the number of pairs is more than 1.\n"));
			for(int i=out.freqAmpPairList().size()-1; i>=0; i--)
			{
				if(2.0*out.freqAmpPairList().at(i).second < out.freqAmpPairUncList().at(i).second)
				{
					log.append(QString("Removing Doppler pair %1 because its amplitude (%2) is less than half its uncertainty (%3).\n").arg(i)
							 .arg(QString::number(out.freqAmpPairList().at(i).second,'f',2))
							.arg(QString::number(out.freqAmpPairUncList().at(i).second,'f',2)));
					dpParams.removeAt(i);
					spurious = true;
					break;
				}

				if(out.allFitParams().at(5+3*i) < out.allFitUncertainties().at(5+3*i))
				{
					log.append(QString("Removing Doppler pair %1 because its alpha (%2) is less than its uncertainty (%3).\n").arg(i)
							 .arg(QString::number(out.allFitParams().at(5+3*i),'f',2))
							.arg(QString::number(out.allFitUncertainties().at(5+3*i),'f',2)));
					dpParams.removeAt(i);
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
				log.append(QString("Fit gave too small a linewidth (%1). Removing weakest pair.\n")
						 .arg(QString::number(out.allFitParams().at(3),'f',5)));
				dpParams.removeLast();
				iterations = 50;
				continue;
			}

			if(fabs(out.allFitParams().at(2)-splitting) > 0.5*splitting)
			{
				log.append(QString("Fit gave a splitting more than 50% away from guess (%1). Removing weakest pair.\n")
						 .arg(QString::number(out.allFitParams().at(2),'f',5)));
				dpParams.removeLast();
				iterations = 50;
				continue;
			}

			if(dpParams.size()>1)
			{
				if(out.allFitParams().at(3) < out.allFitUncertainties().at(3))
				{
					log.append(QString("Fit linewidth (%1) is less than uncertainty (%2). Removing weakest pair.\n")
							 .arg(QString::number(out.allFitParams().at(3),'f',5))
							 .arg(QString::number(out.allFitUncertainties().at(3),'f',5)));
					dpParams.removeLast();
					iterations = 50;
					continue;
				}

				if(out.allFitParams().at(2) < out.allFitUncertainties().at(2))
				{
					log.append(QString("Fit splitting (%1) is less than uncertainty (%2). Removing weakest pair.\n")
							 .arg(QString::number(out.allFitParams().at(2),'f',5))
							 .arg(QString::number(out.allFitUncertainties().at(2),'f',5)));
					dpParams.removeLast();
					iterations = 50;
					continue;
				}
			}

			//if all doppler pair parameters look good, check single peaks
			//check amplitudes
			if(out.freqAmpSingleList().size()>0)
				log.append(QString("Checking single peak parameters....\n"));
			for(int i=out.freqAmpSingleList().size()-1; i>=0; i--)
			{
				if(out.freqAmpSingleList().at(i).second <  5.0*(blData.at(2) + blData.at(3)*(out.freqAmpSingleList().at(i).first-out.probeFreq()))
						|| out.freqAmpSingleList().at(i).second> 100.0
						|| out.freqAmpSingleList().at(i).second < 0.0)
				{
					log.append(QString("Removing single peak %1 because its amplitude (%2) is bad.\n").arg(i)
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
					log.append(QString("Removing single peak %1 because its amplitude (%2) is less than its uncertainty (%3)\n").arg(i)
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
					log.append(QString("Removing single peak %1 because its amplitude (%2) is less than its uncertainty (%3)\n").arg(i)
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
					log.append(QString("Removing single peak %1 because its amplitude (%2) is less than its uncertainty (%3)\n").arg(i)
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
				log.append(QString("Fit linewidth (%1) is less than uncertainty (%2). Removing weakest peak.\n")
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

	log.append(QString("Fitting complete. Iterations = %1 chisq = %2\n").arg(out.iterations())
			 .arg(QString::number(out.chisq(),'e',3)));
	out.setDelay(ftw.delay());
	out.setHpf(ftw.hpf());
	out.setExp(ftw.exp());
    out.setRdc(ftw.removeDC());
    out.setZpf(ftw.autoPad());
    out.setUseWindow(ftw.isUseWindow());
	out.setTemperature(d_temperature);
	out.setBufferGas(d_bufferGas);
	out.appendToLog(log);
    out.setLineShape(lsf);
	out.save(s.number());
	emit fitComplete(out);
	return out;
}
