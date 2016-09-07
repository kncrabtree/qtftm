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
    if(!ftw.isUseWindow())
        calcCoefs(7,4);
    QList<QPair<QPointF,double> > peakList = findPeaks(ftBl,blData.at(2),blData.at(3));
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
    double maxSnr = 0.0;
	for(int i=0; i<peakList.size(); i++)
	{
        maxSnr = qMax(peakList.at(i).second,maxSnr);
        out.appendToLog(QString("%1\t%2\t%3\t%4").arg(i).arg(QString::number(peakList.at(i).first.x()+f.probeFreq(),'f',3))
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

	//at this point, there are 3 possibilities:
	//Pairs only, Mixed pairs and singles, and singles only
	//during fitting, we may transition from mixed->pairs only, or from mixed->singles only

    //strategy: first, fit to line, and determine chi squared.
    //then add in any single peaks, see if chi squared improves, starting from strongest to weakest.
    //continue until chi squared is not improving significantly
    //then add in doppler pairs one at a time from strongest to weakest, continuing while the chi squared keeps improving

    QList<FitResult::DopplerPairParameters> fitDp;
    QList<QPointF> fitSingle;


    if(lsf == FitResult::Lorentzian)
        out.appendToLog(QString("Using Lorentzian lineshape."));
    else if(lsf == FitResult::Gaussian)
        out.appendToLog(QString("Using Gaussian lineshape."));

    QList<double> commonParams;
    commonParams << blData.at(0) << blData.at(1) << splitting << width;

    if(singlePeaks.size() == 0 && dpParams.size() == 0)
        out.appendToLog(QString("No peaks found. Fitting to line."));
    else
        out.appendToLog(QString("Performing initial linear fit to assess chi squared."));
    out = Analysis::fitLine(out,ftPad,fid.probeFreq());

    FitResult lastFit = out;

    bool done = false;
    int iterations = 50;
    bool refit = false;
    int stage = 0;
	while(!done)
	{
        if(!refit)
        {
            iterations = 100;

            if(!singlePeaks.isEmpty())
            {
                stage = 1;
                fitSingle.append(singlePeaks.takeFirst());
                out.appendToLog(QString("Adding strongest single peak to fit....\nX = %1 MHz, Y = %2").arg(fitSingle.last().x()+f.probeFreq(),0,'f',3).arg(fitSingle.last().y(),0,'f',3));
            }
            else if(!dpParams.isEmpty())
            {
                stage = 2;
                fitDp.append(dpParams.takeFirst());
                out.appendToLog(QString("Adding strongest Doppler pair to fit....\nX = %1 MHz, A = %2, alpha = %3").arg(fitDp.last().centerFreq+f.probeFreq(),0,'f',3)
                                .arg(fitDp.last().amplitude,0,'f',3).arg(fitDp.last().alpha,0,'f',3));
            }
            else
                break;
        }

        refit = false;
        out = dopplerFit(ftPad,out,commonParams,fitDp,fitSingle,iterations);

        if(out.category() == FitResult::Fail)
        {
            if(out.iterations() == iterations && iterations < 1000)
            {
                refit = true;
                out.appendToLog(QString("Fit did not converge after %1 iterations. Increasing to %2.").arg(iterations).arg(iterations*2));
                iterations *= 2;
                continue;
            }
            else
            {
                lastFit.setLogText(out.log());
                lastFit.appendToLog(QString("Fit failed, falling back to previous result."));
                out = lastFit;
                break;
            }
        }
        else
        {
            if(out.iterations() == iterations && iterations < 1000)
            {
                refit = true;
                out.appendToLog(QString("Fit did not converge after %1 iterations. Increasing to %2.").arg(iterations).arg(iterations*2));
                iterations *= 2;
                continue;
            }

            double chisqLimit = 0.85;
            if(fitSingle.isEmpty() && fitDp.size() == 1 && maxSnr < 10.0)
                chisqLimit = 0.75;
            if(out.chisq() < chisqLimit*lastFit.chisq())
            {
                lastFit = out;
                if(singlePeaks.isEmpty() && dpParams.isEmpty())
                    done = true;
                else if(stage == 1)
                {
                    commonParams[0] = out.allFitParams().at(0);
                    commonParams[1] = out.allFitParams().at(1);
                    commonParams[3] = out.width();

                    int index = fitSingle.size()-1;
                    fitSingle[index].setX(out.freqAmpSingleList().at(index).first);
                    fitSingle[index].setY(out.freqAmpSingleList().at(index).second);
                }
                else if(stage == 2)
                {
                    commonParams[0] = out.allFitParams().at(0);
                    commonParams[1] = out.allFitParams().at(1);
                    commonParams[2] = out.splitting();
                    commonParams[3] = out.width();

                    int index = fitDp.size()-1;
                    fitDp[index] = out.dopplerParameters(index);

                    if(fitDp.size() == 1)
                    {
                        //reassess potential Doppler pairs with tighter tolerance
                        out.appendToLog(QString("Reassessing Doppler pairs with tighter tolerance."));
                        dpParams = Analysis::estimateDopplerCenters(peakList,out.splitting(),ftSpacing,0.05);
                        if(!dpParams.isEmpty())
                            dpParams.removeFirst();

                        out.appendToLog(QString("Found %1 additional possible pairs.").arg(dpParams.size()));

                        if(dpParams.isEmpty())
                            done = true;
                    }
                }

            }
            else
            {
                lastFit.setLogText(out.log());
                lastFit.appendToLog(QString("Not enough improvement in chi squared (last: %1 vs current: %2). Going back to previous fit...").arg(lastFit.chisq(),0,'e',4).arg(out.chisq(),0,'e',4));
                out = lastFit;

                if(!singlePeaks.isEmpty())
                {
                    out.appendToLog(QString("Clearing remaining single peaks..."));
                    singlePeaks.clear();
                    if(dpParams.isEmpty())
                        done = true;
                }
                else if(!dpParams.isEmpty())
                {
                    out.appendToLog(QString("Clearing remaining Doppler pairs..."));
                    dpParams.clear();
                    done = true;
                }

                if(stage == 1)
                    fitSingle.removeLast();
                else if(stage == 2)
                {
                    fitDp.removeLast();
                    done = true;
                }
            }
        }
	}

    out.appendToLog(QString("Fit complete. Chi squared = %1").arg(out.chisq(),0,'e',4));
    out.appendToLog(QString("Total Doppler pairs: %1. Total single peaks: %2").arg(out.freqAmpPairList().size()).arg(out.freqAmpSingleList().size()));
	out.save(s.number());
	emit fitComplete(out);
	return out;
}
