#include "analysis.h"
#include <math.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_sf.h>
#include "fitresult.h"

double Analysis::lor(double x, double center, double fwhm)
{
    return 1.0/(1.0 + 4.0*(x-center)*(x-center)/(fwhm*fwhm));
}

double Analysis::gauss(double x, double center, double fwhm)
{
    double x_ = x-center;
    double num = x_*x_;
    double den = fwhm*fwhm/twoLogTwo;

    double out = gsl_sf_exp(-num/den);
    if(isnan(out))
        out = 0.0;
    return out;
}

int Analysis::lorDopplerPair_f(const gsl_vector *x, void *data, gsl_vector *f)
{
    //for doppler pair fitting, we need to know how many pairs there are. Usually only 1, but maybe more
    //the function is y0 + slope*x + sum_{numPeaks} { amp*alpha*lor(x,x0-split/2,w) + amp*(1-alpha)*lor(x,x0+split/2,w) }
    //lor is a peak-normalized lorentzian lineshape function
    //order of parameters is y0, slope, split, width, {amp,alpha,x0}_0 ... {amp,alpha,x0}_(numPeaks-1)

    int numPeaks = (x->size-4)/3;

    double y0 = gsl_vector_get(x,0);
    double slope = gsl_vector_get(x,1);
    double split = gsl_vector_get(x,2);
    double width = gsl_vector_get(x,3);

    QVector<double> amp, alpha, center;
    for(int i=0;i<numPeaks;i++)
    {
	   amp.append(gsl_vector_get(x,4+3*i));
	   alpha.append(gsl_vector_get(x,4+3*i+1));
	   center.append(gsl_vector_get(x,4+3*i+2));
    }

    QVector<QPointF> *d = (QVector<QPointF> *)data;
    for(int i=0;i<d->size();i++)
    {
	   //calculate expected value
	   double xVal = d->at(i).x();
	   double val = y0 + slope*xVal;
	   for(int j=0;j<numPeaks;j++)
	   {
		  double redLor = lor(xVal,center.at(j)-split/2.0,width);
		  double blueLor = lor(xVal,center.at(j)+split/2.0,width);
		  val += 2.0*amp.at(j)*(alpha.at(j)*redLor + (1.0-alpha.at(j))*blueLor);
	   }

	   //populate output vector with calc-obs
	   gsl_vector_set(f,i,val - d->at(i).y());
    }

    return GSL_SUCCESS;
}


int Analysis::lorDopplerPair_df(const gsl_vector *x, void *data, gsl_matrix *J)
{
    //calculate jacobian matrix (matrix of derivatives)
    int numPeaks = (x->size-4)/3;

//	double y0 = gsl_vector_get(x,0);
//	double slope = gsl_vector_get(x,1);
    double split = gsl_vector_get(x,2);
    double width = gsl_vector_get(x,3);

    QVector<double> amp, alpha, center;
    for(int i=0;i<numPeaks;i++)
    {
	   amp.append(gsl_vector_get(x,4+3*i));
	   alpha.append(gsl_vector_get(x,4+3*i+1));
	   center.append(gsl_vector_get(x,4+3*i+2));
    }

    QVector<QPointF> *d = (QVector<QPointF> *)data;

    for(int i=0;i<d->size();i++)
    {
	   double xVal = d->at(i).x();

	   //These derivatives were calculated with Mathematica
	   double dY0 = 1.0;
	   double dSlope = xVal;
	   double dSplit = 0.0, dWidth = 0.0;
	   for(int j=0;j<numPeaks;j++)
	   {
		  double A = amp.at(j);
		  double x0 = center.at(j);
		  double al = alpha.at(j);

		  double redLor = lor(xVal,x0-split/2.0,width);
		  double blueLor = lor(xVal,x0+split/2.0,width);

		  dSplit += -(8.0*A/(width*width))*(al*(xVal-(x0-split/2.0))*(redLor*redLor)
								   - (1.0-al)*(xVal-(x0+split/2.0))*(blueLor*blueLor));
		  dWidth += (16.0*A/(width*width*width))*(al*(xVal-(x0-split/2.0))*(xVal-(x0-split/2.0))*(redLor*redLor)
								  + (1.0-al)*(xVal-(x0+split/2.0))*(xVal-(x0+split/2.0))*(blueLor*blueLor));

		  double dAi = 2.0*(al*redLor + (1.0-al)*blueLor);
		  double dAlphai = 2.0*A*(redLor - blueLor);
		  double dX0i = (16.0*A/(width*width))*(al*(xVal-(x0-split/2.0))*(redLor*redLor)
								+ (1.0-al)*(xVal-(x0+split/2.0))*(blueLor*blueLor));

		  gsl_matrix_set(J,i,4+3*j,dAi);
		  gsl_matrix_set(J,i,4+3*j+1,dAlphai);
		  gsl_matrix_set(J,i,4+3*j+2,dX0i);
	   }

	   gsl_matrix_set(J,i,0,dY0);
	   gsl_matrix_set(J,i,1,dSlope);
	   gsl_matrix_set(J,i,2,dSplit);
	   gsl_matrix_set(J,i,3,dWidth);

    }

    return GSL_SUCCESS;
}


int Analysis::lorDopplerPair_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
    lorDopplerPair_f(x,data,f);
    lorDopplerPair_df(x,data,J);

    return GSL_SUCCESS;
}


int Analysis::lorDopplerMixed_f(const gsl_vector *x, void *data, gsl_vector *f)
{
	MixedDopplerData *d = static_cast<MixedDopplerData*>(data);

	double y0 = gsl_vector_get(x,0);
	double slope = gsl_vector_get(x,1);
	double split = gsl_vector_get(x,2);
	double width = gsl_vector_get(x,3);

	QVector<double> amps, alphas, centers;
	amps.reserve(d->numPairs);
	alphas.reserve(d->numPairs);
	centers.reserve(d->numPairs);
	for(int i=0;i<d->numPairs;i++)
	{
		amps.append(gsl_vector_get(x,4+3*i));
		alphas.append(gsl_vector_get(x,5+3*i));
		centers.append(gsl_vector_get(x,6+3*i));
	}

	QVector<double> singleAmps, singleCenters;
	singleAmps.reserve(d->numSinglePeaks);
	singleCenters.reserve(d->numSinglePeaks);
	for(int i=0; i<d->numSinglePeaks; i++)
	{
		singleAmps.append(gsl_vector_get(x,4+3*d->numPairs+2*i));
		singleCenters.append(gsl_vector_get(x,5+3*d->numPairs+2*i));
	}

	for(int i=0;i<d->data.size();i++)
	{
		double xVal = d->data.at(i).x();
		double val = y0 + slope*xVal;

		for(int j=0; j<d->numPairs; j++)
		{
			double a = amps.at(j);
			double al = alphas.at(j);
			double x0 = centers.at(j);

			double redLor = lor(xVal,x0-split/2.0,width);
			double blueLor = lor(xVal,x0+split/2.0,width);

			val += 2.0*a*(al*redLor + (1.0-al)*blueLor);
		}

		for(int j=0; j<d->numSinglePeaks; j++)
		{
			double a = singleAmps.at(j);
			double x0 = singleCenters.at(j);

			val += a*lor(xVal,x0,width);
		}

		gsl_vector_set(f,i,val - d->data.at(i).y());
	}

	return GSL_SUCCESS;
}


int Analysis::lorDopplerMixed_df(const gsl_vector *x, void *data, gsl_matrix *J)
{
	MixedDopplerData *d = static_cast<MixedDopplerData*>(data);

//	double y0 = gsl_vector_get(x,0);
//	double slope = gsl_vector_get(x,1);
	double split = gsl_vector_get(x,2);
	double width = gsl_vector_get(x,3);

	QVector<double> amps, alphas, centers;
	amps.reserve(d->numPairs);
	alphas.reserve(d->numPairs);
	centers.reserve(d->numPairs);
	for(int i=0;i<d->numPairs;i++)
	{
		amps.append(gsl_vector_get(x,4+3*i));
		alphas.append(gsl_vector_get(x,5+3*i));
		centers.append(gsl_vector_get(x,6+3*i));
	}

	QVector<double> singleAmps, singleCenters;
	singleAmps.reserve(d->numSinglePeaks);
	singleCenters.reserve(d->numSinglePeaks);
	for(int i=0; i<d->numSinglePeaks; i++)
	{
		singleAmps.append(gsl_vector_get(x,4+3*d->numPairs+2*i));
		singleCenters.append(gsl_vector_get(x,5+3*d->numPairs+2*i));
	}

	for(int i=0; i<d->data.size(); i++)
	{
		double xVal = d->data.at(i).x();
		double dy0 = 1;
		double dSlope = xVal;

		double dSplit = 0.0, dWidth = 0.0;

		for(int j=0; j<d->numPairs; j++)
		{
			double A = amps.at(j);
			double x0 = centers.at(j);
			double al = alphas.at(j);

			double redLor = lor(xVal,x0-split/2.0,width);
			double blueLor = lor(xVal,x0+split/2.0,width);

			dSplit += -(8.0*A/(width*width))*(al*(xVal-(x0-split/2.0))*(redLor*redLor)
									 - (1.0-al)*(xVal-(x0+split/2.0))*(blueLor*blueLor));
			dWidth += (16.0*A/(width*width*width))*(al*(xVal-(x0-split/2.0))*(xVal-(x0-split/2.0))*(redLor*redLor)
									+ (1.0-al)*(xVal-(x0+split/2.0))*(xVal-(x0+split/2.0))*(blueLor*blueLor));

			double dAi = 2.0*(al*redLor + (1.0-al)*blueLor);
			double dAlphai = 2.0*A*(redLor - blueLor);
			double dX0i = (16.0*A/(width*width))*(al*(xVal-(x0-split/2.0))*(redLor*redLor)
								   + (1.0-al)*(xVal-(x0+split/2.0))*(blueLor*blueLor));

			gsl_matrix_set(J,i,4+3*j,dAi);
			gsl_matrix_set(J,i,5+3*j,dAlphai);
			gsl_matrix_set(J,i,6+3*j,dX0i);
		}

		for(int j=0; j<d->numSinglePeaks; j++)
		{
			double A = singleAmps.at(j);
			double x0 = singleCenters.at(j);

			double dX0i = 8.0*(xVal-x0)/(width*width)*lor(xVal,x0,width);
			double dAi = lor(xVal,x0,width);
			dWidth += 8.0*A*(xVal-x0)*(xVal-x0)/(width*width*width)*lor(xVal,x0,width);

			gsl_matrix_set(J,i,4+3*d->numPairs+2*j,dAi);
			gsl_matrix_set(J,i,5+3*d->numPairs+2*j,dX0i);
		}

		gsl_matrix_set(J,i,0,dy0);
		gsl_matrix_set(J,i,1,dSlope);
		gsl_matrix_set(J,i,2,dSplit);
		gsl_matrix_set(J,i,3,dWidth);
	}

	return GSL_SUCCESS;
}


int Analysis::lorDopplerMixed_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
    lorDopplerMixed_f(x,data,f);
    lorDopplerMixed_df(x,data,J);

	return GSL_SUCCESS;
}


int Analysis::lorSingle_f(const gsl_vector *x, void *data, gsl_vector *f)
{
	double y0 = gsl_vector_get(x,0);
	double slope = gsl_vector_get(x,1);
	double width = gsl_vector_get(x,2);

	int numPeaks = (x->size-3)/2;

	QVector<double> ampList,centerList;
	ampList.reserve(numPeaks);
	centerList.reserve(numPeaks);
	for(int i=0; i<numPeaks; i++)
	{
		ampList.append(gsl_vector_get(x,3+2*i));
		centerList.append(gsl_vector_get(x,4+2*i));
	}

	QVector<QPointF> *d = static_cast<QVector<QPointF> *>(data);

	for(int i=0; i<d->size(); i++)
	{
		double xVal = d->at(i).x();
		double val = y0 + slope*xVal;

		for(int j=0; j<numPeaks; j++)
		{
			double A = ampList.at(j);
			double x0 = centerList.at(j);

			val += A*lor(xVal,x0,width);
		}

		gsl_vector_set(f,i,val - d->at(i).y());
	}

	return GSL_SUCCESS;
}


int Analysis::lorSingle_df(const gsl_vector *x, void *data, gsl_matrix *J)
{
//	double y0 = gsl_vector_get(x,0);
//	double slope = gsl_vector_get(x,1);
	double width = gsl_vector_get(x,2);

	int numPeaks = (x->size-3)/2;

	QVector<double> ampList,centerList;
	ampList.reserve(numPeaks);
	centerList.reserve(numPeaks);
	for(int i=0; i<numPeaks; i++)
	{
		ampList.append(gsl_vector_get(x,3+2*i));
		centerList.append(gsl_vector_get(x,4+2*i));
	}

	QVector<QPointF> *d = static_cast<QVector<QPointF> *>(data);

	for(int i=0; i<d->size(); i++)
	{
		double xVal = d->at(i).x();
		double dy0 = 1.0;
		double dSlope = xVal;
		double dWidth = 0.0;

		for(int j=0; j<numPeaks; j++)
		{
			double A = ampList.at(j);
			double x0 = centerList.at(j);

			double dX0i = 8.0*(xVal-x0)/(width*width)*lor(xVal,x0,width);
			double dAi = lor(xVal,x0,width);
			dWidth += 8.0*A*(xVal-x0)*(xVal-x0)/(width*width*width)*lor(xVal,x0,width);

			gsl_matrix_set(J,i,3+2*j,dAi);
			gsl_matrix_set(J,i,4+2*j,dX0i);
		}

		gsl_matrix_set(J,i,0,dy0);
		gsl_matrix_set(J,i,1,dSlope);
		gsl_matrix_set(J,i,2,dWidth);

	}

	return GSL_SUCCESS;
}


int Analysis::lorSingle_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
	lorSingle_f(x,data,f);
	lorSingle_df(x,data,J);

	return GSL_SUCCESS;
}

QVector<double> Analysis::extractCoordinateVector(const QVector<QPointF> data, Analysis::Coordinate c)
{
    QVector<double> coordData;
    coordData.reserve(data.size());

    if(c == Xcoord)
    {
	   for(int i=0;i<data.size();i++)
		  coordData.append(data.at(i).x());
    }
    else
    {
	   for(int i=0;i<data.size();i++)
		  coordData.append(data.at(i).y());
    }

    return coordData;
}

double Analysis::median(const QVector<double> data)
{
    if(data.size() == 0)
	   return 0.0;

    if(data.size() == 1)
	   return data.at(0);

    if(data.size() == 2)
	   return (data.at(0)+data.at(1))/2.0;

    QVector<double> sort(data);
    qSort(sort);

    if(sort.size() % 2)
	   return sort.at(sort.size()/2);
    else
	   return (sort.at(sort.size()/2) + sort.at(sort.size()/2+1))/2.0;
}


double Analysis::median(const QVector<QPointF> data, Analysis::Coordinate c)
{
    return median(extractCoordinateVector(data,c));
}


QPointF Analysis::median(const QVector<QPointF> data)
{
    return QPointF(median(extractCoordinateVector(data,Xcoord)),
			    median(extractCoordinateVector(data,Ycoord)));
}


double Analysis::mean(const QVector<double> data)
{
    if(data.size() == 0)
	   return 0.0;

    double sum = 0;
    for(int i=0;i<data.size();i++)
	   sum += data.at(i);
    return sum/(double)data.size();
}


double Analysis::mean(const QVector<QPointF> data, Analysis::Coordinate c)
{
    return mean(extractCoordinateVector(data,c));
}


QPointF Analysis::mean(const QVector<QPointF> data)
{
    return QPointF(mean(extractCoordinateVector(data,Xcoord)),
			    mean(extractCoordinateVector(data,Ycoord)));
}


double Analysis::variance(const QVector<double> data)
{
    if(data.size() < 2)
	   return 0.0;

    double m = mean(data), sumsq = 0.0;
    for(int i=0;i<data.size();i++)
    {
	   double diff = data.at(i)-m;
	   sumsq += diff*diff;
    }

    return sumsq;
}


double Analysis::variance(const QVector<QPointF> data, Analysis::Coordinate c)
{
    return variance(extractCoordinateVector(data,c));
}


QPointF Analysis::variance(const QVector<QPointF> data)
{
    return QPointF(variance(extractCoordinateVector(data,Xcoord)),
			    variance(extractCoordinateVector(data,Ycoord)));
}


double Analysis::stDev(const QVector<double> data)
{
    if(data.size() < 2)
	   return 0.0;

    return sqrt(variance(data)/((double)data.size()-1.0));
}


double Analysis::stDev(const QVector<QPointF> data, Analysis::Coordinate c)
{
    return stDev(extractCoordinateVector(data,c));
}


QPointF Analysis::stDev(const QVector<QPointF> data)
{
    return QPointF(stDev(extractCoordinateVector(data,Xcoord)),
			    stDev(extractCoordinateVector(data,Ycoord)));
}


QVector<QPointF> Analysis::boxcarSmooth(const QVector<QPointF> data, int numPoints)
{
    if(numPoints<1)
	   numPoints = 1;

    if(data.size() == 0)
	   return QVector<QPointF>();

    QVector<QPointF> out;
    out.reserve(data.size());

    for(int i=0;i<data.size();i++)
    {
	   QVector<QPointF> chunk = data.mid(qMax(0,i-numPoints/2),numPoints);
	   double m = mean(chunk,Ycoord);
	   //for the edges, average in zeroes for the points that fall outside the data range
	   m *= (double)chunk.size()/(double)numPoints;
	   out.append(QPointF(data.at(i).x(),m));
    }
    return out;
}


QList<QPair<QPointF, double> > Analysis::findPeaks(const QVector<QPointF> data, double noiseY0, double noiseSlope, double snr)
{
    //calculate number of points to smooth.
    //boxcar smooth data
    QVector<QPointF> smth = boxcarSmooth(data,data.size()/smoothFactor+1);

    //subtract out smoothed wave and use for peakfinding
    for(int i=0;i<smth.size();i++)
	   smth[i].setY(data.at(i).y() - smth.at(i).y());

    QList<QPair<QPointF, double> > out;
    out.reserve(10); //preallocate memory for 10 peaks

    for(int i=1;i<smth.size()-1;i++)
    {
	   double noise = noiseY0 + noiseSlope*smth.at(i).x();
       if(smth.at(i).y()>snr*noise && smth.at(i).y() > smth.at(i-1).y() && smth.at(i).y() > smth.at(i+1).y())
		  out.append(qMakePair(data.at(i),data.at(i).y()/noise)); //note, point from original data array used here
    }

    return out;

}


bool Analysis::isFidSaturated(const Fid f)
{
    for(int i=0;i<f.size();i++)
    {
	   if(fabs(f.at(i)) > fidSaturationLimit)
		  return true;
    }

    return false;
}


Fid Analysis::removeDC(const Fid f)
{
    QVector<double> data = f.toVector();
    double m = mean(data);
    for(int i=0;i<data.size();i++)
	   data[i]-=m;

    Fid out(f);
    out.setData(data);
    return out;
}


QList<double> Analysis::estimateBaseline(const QVector<QPointF> ftData)
{
    //Goal: return a list that contains 4 elements: the y0 and slope of the baseline,
    //and y0 and slope of the noise on top of the baseline as a function of offset freq
    QVector<QPointF> medianBins;
    medianBins.reserve(ftData.size()/baselineMedianBinSize+1);
    QVector<double> weights,stDevs;
    weights.reserve(ftData.size()/baselineMedianBinSize+1);
    stDevs.reserve(ftData.size()/baselineMedianBinSize+1);

    for(int i=0;i<ftData.size();i+=baselineMedianBinSize)
    {
	   QVector<QPointF> temp = ftData.mid(i,baselineMedianBinSize);
	   if(temp.size() == baselineMedianBinSize)
	   {
		  medianBins.append(median(temp));
		  weights.append(1.0/variance(temp,Ycoord));
		  stDevs.append(stDev(temp,Ycoord));
	   }
    }

    //compute median of the medians, then throw out any bins that are >2x that value
    double medianOfMedians = median(medianBins).y();
    QVector<QPointF> baseline, working = medianBins;
    baseline.reserve(medianBins.size());
    QVector<double> blWeights, workingWeights = weights, blStDevs, workingStDevs = stDevs;
    blWeights.reserve(weights.size());
    blStDevs.reserve(stDevs.size());
    int pointsRemoved = 0;
    do
    {
	   baseline.clear();
	   baseline.reserve(working.size());
	   blWeights.clear();
	   blWeights.reserve(workingWeights.size());
	   blStDevs.clear();
	   blStDevs.reserve(workingStDevs.size());
	   pointsRemoved = 0;

	   for(int i=0;i<working.size();i++)
	   {
		  if(working.at(i).y() < 2.0*medianOfMedians)
		  {
			 baseline.append(working.at(i));
			 blWeights.append(workingWeights.at(i));
			 blStDevs.append(workingStDevs.at(i));
		  }
		  else
			 pointsRemoved++;
	   }

	   medianOfMedians = median(baseline,Ycoord);
	   working = baseline;
	   workingWeights = blWeights;
	   workingStDevs = blStDevs;
    }
    while(pointsRemoved > 0);

    QVector<double> blX, blY;
    blX.reserve(baseline.size());
    blY.reserve(baseline.size());
    for(int i=0;i<baseline.size();i++)
    {
	   blX.append(baseline.at(i).x());
	   blY.append(baseline.at(i).y());
    }

    //fit baseline to weighted linear function
    double y0,slope,cv00,cv01,cv11,chisq;
    int success = gsl_fit_wlinear(blX.constData(),1,blWeights.constData(),1,blY.constData(),1,blX.size(),&y0,&slope,&cv00,&cv01,&cv11,&chisq);

    if(success != GSL_SUCCESS)
	   return QList<double>();

    QList<double> out;
    out.append(y0);
    out.append(slope);

    //now fit the standard deviations of those bins to a linear function to estimate noise
    //level as a function of frequency
    success = gsl_fit_linear(blX.constData(),1,blStDevs.constData(),1,blStDevs.size(),&y0,&slope,&cv00,&cv01,&cv11,&chisq);
    if(success != GSL_SUCCESS)
    {
	   //if the fit fails, then we'll just assume the noise is about equal to the
	   //value of the baseline (not an entirely horrible estimate)
	   out.append(out.at(0));
	   out.append(out.at(1));
	   return out;
    }

    out.append(y0);
    out.append(slope);
    return out;
}


QVector<QPointF> Analysis::removeBaseline(const QVector<QPointF> data, double y0, double slope, double probeFreq)
{
	QVector<QPointF> out;
	out.reserve(data.size());
	for(int i=0;i<data.size();i++)
		out.append(QPointF(data.at(i).x(),data.at(i).y() - (y0 + slope*(data.at(i).x()-probeFreq))));

	return out;
}


double Analysis::estimateSplitting(const FitResult::BufferGas &bg, double stagT, double frequency)
{
	double velocity = sqrt(bg.gamma/(bg.gamma-1.0))*sqrt(2.0*GSL_CONST_CGS_BOLTZMANN*stagT/bg.mass);
//	if(bg.name == QString("He"))
//		velocity *= 1.5;

	return (2.0*velocity/GSL_CONST_CGS_SPEED_OF_LIGHT)*frequency;
}


QList<FitResult::DopplerPairParameters> Analysis::estimateDopplerCenters(QList<QPair<QPointF,double> > peakList, double splitting, double ftSpacing)
{
    QList<FitResult::DopplerPairParameters> out;
	out.reserve(peakList.size());
	for(int i=0;i<peakList.size();i++)
	{
		for(int j=i+1;j<peakList.size();j++)
		{
			 // see if splitting is within tolerance
			if(fabs(fabs(peakList.at(i).first.x()-peakList.at(j).first.x())
				   - splitting)/splitting < splittingTolerance ||
					fabs(fabs(peakList.at(i).first.x()-peakList.at(j).first.x())
									   - splitting) < 2.5*ftSpacing)
			{
				double amp = (peakList.at(i).first.y()+peakList.at(j).first.y())/2.0;
				double alpha = peakList.at(i).first.y()/2.0/amp;
				double x0 = (peakList.at(i).first.x() + peakList.at(j).first.x())/2.0;
				double lSkeptical = edgeSkepticalWRTSplitting*splitting;
				double uSkeptical = 1.0 - edgeSkepticalWRTSplitting*splitting;
				double snr = (peakList.at(i).second + peakList.at(j).second)/2.0;
				if(alpha > alphaTolerance && alpha < (1.0-alphaTolerance))
				{
					//check to make sure there's not another candidate. If there is, prefer the one with alpha closer to 0.5
					for(int k=j+1; k<peakList.size(); k++)
					{
						if(fabs(fabs(peakList.at(i).first.x()-peakList.at(k).first.x())
							   - splitting)/splitting < splittingTolerance ||
								fabs(fabs(peakList.at(i).first.x()-peakList.at(k).first.x())
												   - splitting) < 4.0*ftSpacing)
						{
							double amp2 = (peakList.at(i).first.y()+peakList.at(k).first.y())/2.0;
							double alpha2 = peakList.at(i).first.y()/2.0/amp2;
							double x02 = (peakList.at(i).first.x() + peakList.at(k).first.x())/2.0;
							double snr2 = (peakList.at(i).second + peakList.at(k).second)/2.0;
							if(fabs(0.5-alpha2) < fabs(0.5-alpha))
							{
								amp = amp2;
								alpha = alpha2;
								x0 = x02;
								snr = snr2;
							}
						}
					}

					if( (x0 <= lSkeptical && snr > 5.0)
							|| (x0 > lSkeptical	&& x0 < uSkeptical)
							|| (x0 > uSkeptical && snr > 5.0) )
                        out.append(FitResult::DopplerPairParameters(amp,alpha,x0));
				}
			}
		}
	}

	if(out.size() < 2)
		return out;

	//need to sort by descending amplitude. qSort is ascending...
	qSort(out.begin(),out.end(),&dpAmplitudeLess);
    QList<FitResult::DopplerPairParameters> outSorted;
	outSorted.reserve(out.size());
	for(int i=out.size()-1;i>=0;i--)
		outSorted.append(out.at(i));
	return outSorted;
}


bool Analysis::dpAmplitudeLess(const FitResult::DopplerPairParameters &left, const FitResult::DopplerPairParameters &right)
{
	return left.amplitude < right.amplitude;
}


double Analysis::estimateDopplerLinewidth(const FitResult::BufferGas &bg, double probeFreq, double stagT)
{
	//estimate is based on the beamwaist diameter of cavity and transit tome of molecular beam
	//seems to get within factor of 2....

	double velocity = sqrt(bg.gamma/(bg.gamma-1.0))*sqrt(2.0*GSL_CONST_CGS_BOLTZMANN*stagT/bg.mass);
	double lambda = GSL_CONST_CGS_SPEED_OF_LIGHT/(probeFreq*1.0e6);
	double R = 83.2048, d = 70.0;
	double modeDiameter = 2.0*sqrt(lambda/(2.0*M_PI)*sqrt(d*(2.0*R-d)));
	double transitTime = modeDiameter/velocity;
	double estWidth = (1.0/transitTime)/1.0e6;
	return estWidth;

//	return 4.0*sqrt(8.0*GSL_CONST_CGS_BOLTZMANN*log(2)/
//				 (bg.mass*GSL_CONST_CGS_SPEED_OF_LIGHT*GSL_CONST_CGS_SPEED_OF_LIGHT))
//			*probeFreq;
}


unsigned int Analysis::power2Nplus1(unsigned int n)
{
	//calculates the second power of 2 larger than N
	//NOTE: this is only 32 bits!
	unsigned int pwr = 1;
	while (pwr < n) {
		pwr = pwr << 1;
	}
	pwr = pwr << 1;
	return pwr;
}



FitResult Analysis::dopplerPairFit(const gsl_multifit_fdfsolver_type *solverType, FitResult::LineShape lsf, const QVector<QPointF> data, const double probeFreq, const double y0, const double slope, const double split, const double width, const QList<FitResult::DopplerPairParameters> dpParams, const int maxIterations)
{
	const size_t numPnts = data.size();
	const size_t numParams = 4+dpParams.size()*3;


	gsl_matrix *covar = gsl_matrix_alloc(numParams,numParams);
	gsl_multifit_function_fdf fitFunction;

	QVector<QPointF> dataCopy(data);

    if(lsf == FitResult::Lorentzian)
    {
        fitFunction.f = &lorDopplerPair_f;
        fitFunction.df = &lorDopplerPair_df;
        fitFunction.fdf = &lorDopplerPair_fdf;
    }
    else if(lsf == FitResult::Gaussian)
    {
        fitFunction.f = &gaussDopplerPair_f;
        fitFunction.df = &gaussDopplerPair_df;
        fitFunction.fdf = &gaussDopplerPair_fdf;
    }
	fitFunction.n = numPnts;
	fitFunction.p = numParams;
	fitFunction.params = &dataCopy;

	gsl_multifit_fdfsolver *solver = gsl_multifit_fdfsolver_alloc(solverType,numPnts,numParams);
	gsl_vector *params = gsl_vector_alloc(numParams);

	gsl_vector_set(params,0,y0);
	gsl_vector_set(params,1,slope);
	gsl_vector_set(params,2,split);
	gsl_vector_set(params,3,width);
	for(int i=0;i<dpParams.size();i++)
	{
		gsl_vector_set(params,4+3*i,dpParams.at(i).amplitude);
		gsl_vector_set(params,5+3*i,dpParams.at(i).alpha);
		gsl_vector_set(params,6+3*i,dpParams.at(i).centerFreq);
	}

	gsl_multifit_fdfsolver_set(solver,&fitFunction,params);

	int iter = 0;
    int status;
	do
	{
		iter++;
		status = gsl_multifit_fdfsolver_iterate(solver);

        if(status)
            break;

        status = gsl_multifit_test_delta(solver->dx,solver->x,0.0,1e-4);


    } while(status == GSL_CONTINUE && iter < maxIterations);

	gsl_multifit_covar(solver->J,0.0,covar);

	double chi = gsl_blas_dnrm2(solver->f);
	double dof = numPnts - numParams;

    FitResult::FitterType t = FitResult::DopplerPair;
	FitResult result(t,FitResult::Success);
	result.setStatus(status);
	result.setProbeFreq(probeFreq);
	result.setIterations(iter);
	result.setChisq(chi*chi/dof);
	result.setFitParameters(solver->x,covar,0);

	gsl_multifit_fdfsolver_free(solver);
	gsl_matrix_free(covar);
	gsl_vector_free(params);

	return result;
}


FitResult Analysis::dopplerMixedFit(const gsl_multifit_fdfsolver_type *solverType, FitResult::LineShape lsf, const QVector<QPointF> data, const double probeFreq, const double y0, const double slope, const double split, const double width, const QList<FitResult::DopplerPairParameters> dpParams, const QList<QPointF> singleParams, const int maxIterations)
{
	const size_t numPnts = data.size();
	const size_t numParams = 4+dpParams.size()*3+2*singleParams.size();

	gsl_matrix *covar = gsl_matrix_alloc(numParams,numParams);
	gsl_multifit_function_fdf fitFunction;

	MixedDopplerData fitData;
	fitData.data = data;
	fitData.numPairs = dpParams.size();
	fitData.numSinglePeaks = singleParams.size();

    if(lsf == FitResult::Lorentzian)
    {
        fitFunction.f = &lorDopplerMixed_f;
        fitFunction.df = &lorDopplerMixed_df;
        fitFunction.fdf = &lorDopplerMixed_fdf;
    }
    else if(lsf == FitResult::Gaussian)
    {
        fitFunction.f = &gaussDopplerMixed_f;
        fitFunction.df = &gaussDopplerMixed_df;
        fitFunction.fdf = &gaussDopplerMixed_fdf;
    }
	fitFunction.n = numPnts;
	fitFunction.p = numParams;
	fitFunction.params = &fitData;

	gsl_multifit_fdfsolver *solver = gsl_multifit_fdfsolver_alloc(solverType,numPnts,numParams);
	gsl_vector *params = gsl_vector_alloc(numParams);

	gsl_vector_set(params,0,y0);
	gsl_vector_set(params,1,slope);
	gsl_vector_set(params,2,split);
	gsl_vector_set(params,3,width);
	for(int i=0;i<dpParams.size();i++)
	{
		gsl_vector_set(params,4+3*i,dpParams.at(i).amplitude);
		gsl_vector_set(params,5+3*i,dpParams.at(i).alpha);
		gsl_vector_set(params,6+3*i,dpParams.at(i).centerFreq);
	}
	for(int i=0;i<singleParams.size();i++)
	{
		gsl_vector_set(params,4+3*dpParams.size()+2*i,singleParams.at(i).y());
		gsl_vector_set(params,5+3*dpParams.size()+2*i,singleParams.at(i).x());
	}

	gsl_multifit_fdfsolver_set(solver,&fitFunction,params);

	int iter = 0;
	int status;
	do
	{
		iter++;
		status = gsl_multifit_fdfsolver_iterate(solver);

		if(status)
			break;

		status = gsl_multifit_test_delta(solver->dx,solver->x,0.0,1e-4);

	} while(status == GSL_CONTINUE && iter < maxIterations);

	gsl_multifit_covar(solver->J,0.0,covar);

	double chi = gsl_blas_dnrm2(solver->f);
	double dof = numPnts - numParams;

    FitResult::FitterType t = FitResult::Mixed;
	FitResult result(t,FitResult::Success);
	result.setStatus(status);
	result.setProbeFreq(probeFreq);
	result.setIterations(iter);
	result.setChisq(chi*chi/dof);
	result.setFitParameters(solver->x,covar,fitData.numSinglePeaks);

	gsl_multifit_fdfsolver_free(solver);
	gsl_matrix_free(covar);
	gsl_vector_free(params);

	return result;
}


FitResult Analysis::singleFit(const gsl_multifit_fdfsolver_type *solverType, FitResult::LineShape lsf, const QVector<QPointF> data, const double probeFreq, const double y0, const double slope, const double width, const QList<QPointF> singleParams, const int maxIterations)
{
	const size_t numPnts = data.size();
	const size_t numParams = 3+singleParams.size()*2;

	gsl_matrix *covar = gsl_matrix_alloc(numParams,numParams);
	gsl_multifit_function_fdf fitFunction;

	QVector<QPointF> dataCopy(data);

    if(lsf == FitResult::Lorentzian)
    {
        fitFunction.f = &lorSingle_f;
        fitFunction.df = &lorSingle_df;
        fitFunction.fdf = &lorSingle_fdf;
    }
    else if(lsf == FitResult::Gaussian)
    {
        fitFunction.f = &gaussSingle_f;
        fitFunction.df = &gaussSingle_df;
        fitFunction.fdf = &gaussSingle_fdf;
    }
	fitFunction.n = numPnts;
	fitFunction.p = numParams;
	fitFunction.params = &dataCopy;

	gsl_multifit_fdfsolver *solver = gsl_multifit_fdfsolver_alloc(solverType,numPnts,numParams);
	gsl_vector *params = gsl_vector_alloc(numParams);

	gsl_vector_set(params,0,y0);
	gsl_vector_set(params,1,slope);
	gsl_vector_set(params,2,width);
	for(int i=0;i<singleParams.size();i++)
	{
		gsl_vector_set(params,3+2*i,singleParams.at(i).y());
		gsl_vector_set(params,4+2*i,singleParams.at(i).x());
	}

	gsl_multifit_fdfsolver_set(solver,&fitFunction,params);

	int iter = 0;
	int status;
	do
	{
		iter++;
		status = gsl_multifit_fdfsolver_iterate(solver);

		if(status)
			break;

		status = gsl_multifit_test_delta(solver->dx,solver->x,0.0,1e-4);

	} while(status == GSL_CONTINUE && iter < maxIterations);

	gsl_multifit_covar(solver->J,0.0,covar);

	double chi = gsl_blas_dnrm2(solver->f);
	double dof = numPnts - numParams;

    FitResult::FitterType t = FitResult::Single;
	FitResult result(t,FitResult::Success);
	result.setStatus(status);
	result.setIterations(iter);
	result.setProbeFreq(probeFreq);
	result.setChisq(chi*chi/dof);
	result.setFitParameters(solver->x,covar,singleParams.size());

	gsl_multifit_fdfsolver_free(solver);
	gsl_matrix_free(covar);
	gsl_vector_free(params);

	return result;
}


FitResult Analysis::fitLine(const FitResult &in, QVector<QPointF> data, double probeFreq)
{
	//fit to a line and exit
	gsl_vector *yData;
	yData = gsl_vector_alloc(data.size());
	for(int i=0; i<data.size(); i++)
		gsl_vector_set(yData,i,data.at(i).y());

	gsl_multifit_robust_workspace *ws = gsl_multifit_robust_alloc(gsl_multifit_robust_bisquare,data.size(),2);
	gsl_matrix *X = gsl_matrix_alloc(data.size(),2);
	gsl_matrix *covar = gsl_matrix_alloc(2,2);
	gsl_vector *c = gsl_vector_alloc(2);

	for(int i=0; i<data.size(); i++)
	{
		double xVal = data.at(i).x();
		gsl_matrix_set(X,i,0,1.0);
		gsl_matrix_set(X,i,1,xVal);
	}

    // if there's an error in there, it will crash the program
    gsl_set_error_handler_off();

	int success = gsl_multifit_robust(X,yData,c,covar,ws);

    FitResult out(in);
    out.setCategory(FitResult::NoPeaksFound);
    out.setType(FitResult::RobustLinear);
    if(success == GSL_SUCCESS) {
        gsl_multifit_robust_stats stats = gsl_multifit_robust_statistics(ws);
        out.setStatus(success);
        out.setIterations(1);
        out.setProbeFreq(probeFreq);
        out.setChisq(stats.sse*stats.sse/stats.dof);
        out.setFitParameters(c,covar);
    }


	gsl_vector_free(yData);
	gsl_vector_free(c);
	gsl_matrix_free(X);
	gsl_matrix_free(covar);
	gsl_multifit_robust_free(ws);

	return out;
}


FitResult::BufferGas Analysis::bgFromString(const QString bgName)
{
    FitResult::BufferGas out = FitResultBG::bufferNe;

	if(bgName.isEmpty())
		return out;

	//default buffer gas is Ne. Check to see if the bgname contains something different
	if(bgName.contains(QString("H2"),Qt::CaseSensitive))
        out = FitResultBG::bufferH2;
	if(bgName.contains(QString("He"),Qt::CaseSensitive))
        out = FitResultBG::bufferHe;
	if(bgName.contains(QString("N2"),Qt::CaseSensitive))
        out = FitResultBG::bufferN2;
	if(bgName.contains(QString("O2"),Qt::CaseSensitive))
        out = FitResultBG::bufferO2;
	if(bgName.contains(QString("Ne"),Qt::CaseSensitive))
        out = FitResultBG::bufferNe;
	if(bgName.contains(QString("Ar"),Qt::CaseSensitive))
        out = FitResultBG::bufferAr;
	if(bgName.contains(QString("Kr"),Qt::CaseSensitive))
        out = FitResultBG::bufferKr;
	if(bgName.contains(QString("Xe"),Qt::CaseSensitive))
        out = FitResultBG::bufferXe;

	return out;
}


void Analysis::estimateDopplerPairAmplitude(const QVector<QPointF> ft, DopplerPair *dp, QPair<double, double> baseline)
{
    if(ft.size() < 20 || !dp)
        return;

    double probeFreq = ft.at(0).x();

    double amp1 = 0.0, amp2 = 0.0;
    double x1 = dp->center()-dp->splitting(), x2 = dp->center()+dp->splitting();

    for(int i=0; i+1 < ft.size(); i++)
    {
        if(x1 > ft.at(i).x() && x1 <= ft.at(i+1).x())
            amp1 = qMax(ft.at(i).y(),ft.at(i+1).y());

        if(x2 > ft.at(i).x() && x2 <= ft.at(i+1).x())
            amp2 = qMax(ft.at(i).y(),ft.at(i+1).y());

        if(amp1 > 0.0 && amp2 > 0.0)
            break;
    }

    double blAmp = baseline.first + baseline.second*(dp->center() - probeFreq);

    dp->setAmplitude( (amp1+amp2)/2.0 - blAmp );
}


Fid Analysis::parseWaveform(const QByteArray d, double probeFreq)
{
    //the byte array consists of a waveform prefix with data about the scaling, etc, followed by a block of data preceded by #xyyyy (number of y characters = x)
    //to parse, first find the hash and split the prefix from the total array, then split the prefix on semicolons
    int hashIndex = d.indexOf('#');
    QByteArray prefix = d.mid(0,hashIndex);
    QList<QByteArray> prefixFields = prefix.split(';');

    //important data from prefix: number of bytes (0), byte format (3), byte order (4), x increment (10), y multiplier (14), y offset (15)
    if(prefixFields.size()<16)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Too few fields. If this problem persists, restart program."),QtFTM::LogWarning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    bool ok = true;
    int n_bytes = prefixFields.at(0).trimmed().toInt(&ok);
    if(!ok || n_bytes < 1 || n_bytes > 2)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid number of bytes per record. If this problem persists, restart program."),QtFTM::LogWarning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    bool n_signed = true;
    if(prefixFields.at(3).trimmed() == QByteArray("RP"))
        n_signed = false;

    QDataStream::ByteOrder n_order = QDataStream::BigEndian;
    if(prefixFields.at(4).trimmed() == QByteArray("LSB"))
        n_order = QDataStream::LittleEndian;

    double xIncr = prefixFields.at(10).trimmed().toDouble(&ok);
    if(!ok || xIncr <= 0.0)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid X spacing. If this problem persists, restart program."),QtFTM::LogWarning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    double yMult = prefixFields.at(14).trimmed().toDouble(&ok);
    if(!ok || yMult == 0.0)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid Y multipier. If this problem persists, restart program."),QtFTM::LogWarning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    double yOffset = prefixFields.at(15).trimmed().toDouble(&ok);
    if(!ok)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid Y offset. If this problem persists, restart program."),QtFTM::LogWarning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    //calculate data stride
    int stride = (int)ceil(500e-9/xIncr);

    //now, locate and extract data block
    int numHeaderBytes = d.mid(hashIndex+1,1).toInt();
    int numDataBytes = d.mid(hashIndex+2,numHeaderBytes).toInt();
    int numRecords = numDataBytes/n_bytes;
    QByteArray dataBlock = d.mid(hashIndex+numHeaderBytes+2,numDataBytes);

    if(dataBlock.size() < numDataBytes)
    {
//        emit logMessage(QString("Could not parse waveform. Incomplete wave. If this problem persists, restart program."),QtFTM::LogWarning);
        return Fid(xIncr*(double)stride,probeFreq,QVector<double>(400));
    }

    //prepare data stream and data vector
    QDataStream ds(&dataBlock,QIODevice::ReadOnly);
    ds.setByteOrder(n_order);
    QVector<double> dat;
    dat.reserve((int)ceil(numRecords/stride));

    for(int i=0; i<numRecords; i++)
    {
        double yVal;
        if(n_bytes == 1)
        {
            if(n_signed)
            {
                qint8 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
            else
            {
                quint8 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
        }
        else
        {
            if(n_signed)
            {
                qint16 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
            else
            {
                quint16 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
        }
        if(!(i%stride))
            dat.append(yVal);
    }

    return Fid(xIncr*(double)stride,probeFreq,dat);

}

int Analysis::gaussDopplerPair_f(const gsl_vector *x, void *data, gsl_vector *f)
{
    int numPeaks = (x->size-4)/3;

    double y0 = gsl_vector_get(x,0);
    double slope = gsl_vector_get(x,1);
    double split = gsl_vector_get(x,2);
    double width = gsl_vector_get(x,3);

    QVector<double> amp, alpha, center;
    for(int i=0;i<numPeaks;i++)
    {
       amp.append(gsl_vector_get(x,4+3*i));
       alpha.append(gsl_vector_get(x,4+3*i+1));
       center.append(gsl_vector_get(x,4+3*i+2));
    }

    QVector<QPointF> *d = (QVector<QPointF> *)data;
    for(int i=0;i<d->size();i++)
    {
       //calculate expected value
       double xVal = d->at(i).x();
       double val = y0 + slope*xVal;
       for(int j=0;j<numPeaks;j++)
       {
          double redGauss = gauss(xVal,center.at(j)-split/2.0,width);
          double blueGauss = gauss(xVal,center.at(j)+split/2.0,width);
          val += 2.0*amp.at(j)*(alpha.at(j)*redGauss + (1.0-alpha.at(j))*blueGauss);
       }

       //populate output vector with calc-obs
       gsl_vector_set(f,i,val - d->at(i).y());
    }

    return GSL_SUCCESS;
}

int Analysis::gaussDopplerPair_df(const gsl_vector *x, void *data, gsl_matrix *J)
{
    //calculate jacobian matrix (matrix of derivatives)
    int numPeaks = (x->size-4)/3;

//	double y0 = gsl_vector_get(x,0);
//	double slope = gsl_vector_get(x,1);
    double split = gsl_vector_get(x,2);
    double width = gsl_vector_get(x,3);

    QVector<double> amp, alpha, center;
    for(int i=0;i<numPeaks;i++)
    {
       amp.append(gsl_vector_get(x,4+3*i));
       alpha.append(gsl_vector_get(x,4+3*i+1));
       center.append(gsl_vector_get(x,4+3*i+2));
    }

    QVector<QPointF> *d = (QVector<QPointF> *)data;

    for(int i=0;i<d->size();i++)
    {
       double xVal = d->at(i).x();

       //These derivatives were calculated with Mathematica
       double dY0 = 1.0;
       double dSlope = xVal;
       double dSplit = 0.0, dWidth = 0.0;
       for(int j=0;j<numPeaks;j++)
       {
          double A = amp.at(j);
          double x0 = center.at(j);
          double al = alpha.at(j);

          double redGauss = gauss(xVal,x0-split/2.0,width);
          double blueGauss = gauss(xVal,x0+split/2.0,width);
          double redExpArg = twoLogTwo*(xVal-x0+split/2.0)*(xVal-x0+split/2.0)/(width*width);
          double blueExpArg = twoLogTwo*(xVal-x0-split/2.0)*(xVal-x0-split/2.0)/(width*width);

          dSplit += -2.0*A*twoLogTwo/(width*width)*(al*(xVal-x0+split/2.0)*redGauss - (1.0-al)*(xVal-x0-split/2.0)*blueGauss);
          dWidth += 4.0*A/width*(al*redGauss*redExpArg + (1.0-al)*blueGauss*blueExpArg);

          double dAi = 2.0*(al*redGauss + (1.0-al)*blueGauss);
          double dAlphai = 2.0*A*(redGauss - blueGauss);
          double dX0i = 4.0*A*twoLogTwo/(width*width)*(al*(xVal-x0+split/2.0)*redGauss + (1.0-al)*(xVal-x0-split/2.0)*blueGauss);

          gsl_matrix_set(J,i,4+3*j,dAi);
          gsl_matrix_set(J,i,4+3*j+1,dAlphai);
          gsl_matrix_set(J,i,4+3*j+2,dX0i);
       }

       gsl_matrix_set(J,i,0,dY0);
       gsl_matrix_set(J,i,1,dSlope);
       gsl_matrix_set(J,i,2,dSplit);
       gsl_matrix_set(J,i,3,dWidth);

    }

    return GSL_SUCCESS;
}

int Analysis::gaussDopplerPair_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
    gaussDopplerPair_f(x,data,f);
    gaussDopplerPair_df(x,data,J);

    return GSL_SUCCESS;
}

int Analysis::gaussDopplerMixed_f(const gsl_vector *x, void *data, gsl_vector *f)
{
    MixedDopplerData *d = static_cast<MixedDopplerData*>(data);

    double y0 = gsl_vector_get(x,0);
    double slope = gsl_vector_get(x,1);
    double split = gsl_vector_get(x,2);
    double width = gsl_vector_get(x,3);

    QVector<double> amps, alphas, centers;
    amps.reserve(d->numPairs);
    alphas.reserve(d->numPairs);
    centers.reserve(d->numPairs);
    for(int i=0;i<d->numPairs;i++)
    {
        amps.append(gsl_vector_get(x,4+3*i));
        alphas.append(gsl_vector_get(x,5+3*i));
        centers.append(gsl_vector_get(x,6+3*i));
    }

    QVector<double> singleAmps, singleCenters;
    singleAmps.reserve(d->numSinglePeaks);
    singleCenters.reserve(d->numSinglePeaks);
    for(int i=0; i<d->numSinglePeaks; i++)
    {
        singleAmps.append(gsl_vector_get(x,4+3*d->numPairs+2*i));
        singleCenters.append(gsl_vector_get(x,5+3*d->numPairs+2*i));
    }

    for(int i=0;i<d->data.size();i++)
    {
        double xVal = d->data.at(i).x();
        double val = y0 + slope*xVal;

        for(int j=0; j<d->numPairs; j++)
        {
            double a = amps.at(j);
            double al = alphas.at(j);
            double x0 = centers.at(j);

            double redGauss = gauss(xVal,x0-split/2.0,width);
            double blueGauss = gauss(xVal,x0+split/2.0,width);

            val += 2.0*a*(al*redGauss + (1.0-al)*blueGauss);
        }

        for(int j=0; j<d->numSinglePeaks; j++)
        {
            double a = singleAmps.at(j);
            double x0 = singleCenters.at(j);

            val += a*lor(xVal,x0,width);
        }

        gsl_vector_set(f,i,val - d->data.at(i).y());
    }

    return GSL_SUCCESS;
}

int Analysis::gaussDopplerMixed_df(const gsl_vector *x, void *data, gsl_matrix *J)
{
    MixedDopplerData *d = static_cast<MixedDopplerData*>(data);

//	double y0 = gsl_vector_get(x,0);
//	double slope = gsl_vector_get(x,1);
    double split = gsl_vector_get(x,2);
    double width = gsl_vector_get(x,3);

    QVector<double> amps, alphas, centers;
    amps.reserve(d->numPairs);
    alphas.reserve(d->numPairs);
    centers.reserve(d->numPairs);
    for(int i=0;i<d->numPairs;i++)
    {
        amps.append(gsl_vector_get(x,4+3*i));
        alphas.append(gsl_vector_get(x,5+3*i));
        centers.append(gsl_vector_get(x,6+3*i));
    }

    QVector<double> singleAmps, singleCenters;
    singleAmps.reserve(d->numSinglePeaks);
    singleCenters.reserve(d->numSinglePeaks);
    for(int i=0; i<d->numSinglePeaks; i++)
    {
        singleAmps.append(gsl_vector_get(x,4+3*d->numPairs+2*i));
        singleCenters.append(gsl_vector_get(x,5+3*d->numPairs+2*i));
    }

    for(int i=0; i<d->data.size(); i++)
    {
        double xVal = d->data.at(i).x();
        double dy0 = 1;
        double dSlope = xVal;

        double dSplit = 0.0, dWidth = 0.0;

        for(int j=0; j<d->numPairs; j++)
        {
            double A = amps.at(j);
            double x0 = centers.at(j);
            double al = alphas.at(j);

            double redGauss = gauss(xVal,x0-split/2.0,width);
            double blueGauss = gauss(xVal,x0+split/2.0,width);
            double redExpArg = twoLogTwo*(xVal-x0+split/2.0)*(xVal-x0+split/2.0)/(width*width);
            double blueExpArg = twoLogTwo*(xVal-x0-split/2.0)*(xVal-x0-split/2.0)/(width*width);

            dSplit += -2.0*A*twoLogTwo/(width*width)*(al*(xVal-x0+split/2.0)*redGauss - (1.0-al)*(xVal-x0-split/2.0)*blueGauss);
            dWidth += 4.0*A/width*(al*redGauss*redExpArg + (1.0-al)*blueGauss*blueExpArg);

            double dAi = 2.0*(al*redGauss + (1.0-al)*blueGauss);
            double dAlphai = 2.0*A*(redGauss - blueGauss);
            double dX0i = 2.0*A*twoLogTwo/(width*width)*(al*(xVal-x0+split/2.0)*redGauss - (1.0-al)*(xVal-x0-split/2.0)*blueGauss);

            gsl_matrix_set(J,i,4+3*j,dAi);
            gsl_matrix_set(J,i,5+3*j,dAlphai);
            gsl_matrix_set(J,i,6+3*j,dX0i);
        }

        for(int j=0; j<d->numSinglePeaks; j++)
        {
            double A = singleAmps.at(j);
            double x0 = singleCenters.at(j);
            double G = gauss(xVal,x0,width);
            double expArg = twoLogTwo*(xVal-x0)*(xVal-x0)/(width*width);

            double dX0i = 2.0*A*G*twoLogTwo*(xVal-x0)/(width*width);
            double dAi = G;
            dWidth += 2.0*A*G*expArg/width;

            gsl_matrix_set(J,i,4+3*d->numPairs+2*j,dAi);
            gsl_matrix_set(J,i,5+3*d->numPairs+2*j,dX0i);
        }

        gsl_matrix_set(J,i,0,dy0);
        gsl_matrix_set(J,i,1,dSlope);
        gsl_matrix_set(J,i,2,dSplit);
        gsl_matrix_set(J,i,3,dWidth);
    }

    return GSL_SUCCESS;
}

int Analysis::gaussDopplerMixed_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
    gaussDopplerMixed_f(x,data,f);
    gaussDopplerMixed_df(x,data,J);

    return GSL_SUCCESS;
}

int Analysis::gaussSingle_f(const gsl_vector *x, void *data, gsl_vector *f)
{
    double y0 = gsl_vector_get(x,0);
    double slope = gsl_vector_get(x,1);
    double width = gsl_vector_get(x,2);

    int numPeaks = (x->size-3)/2;

    QVector<double> ampList,centerList;
    ampList.reserve(numPeaks);
    centerList.reserve(numPeaks);
    for(int i=0; i<numPeaks; i++)
    {
        ampList.append(gsl_vector_get(x,3+2*i));
        centerList.append(gsl_vector_get(x,4+2*i));
    }

    QVector<QPointF> *d = static_cast<QVector<QPointF> *>(data);

    for(int i=0; i<d->size(); i++)
    {
        double xVal = d->at(i).x();
        double val = y0 + slope*xVal;

        for(int j=0; j<numPeaks; j++)
        {
            double A = ampList.at(j);
            double x0 = centerList.at(j);

            val += A*gauss(xVal,x0,width);
        }

        gsl_vector_set(f,i,val - d->at(i).y());
    }

    return GSL_SUCCESS;
}

int Analysis::gaussSingle_df(const gsl_vector *x, void *data, gsl_matrix *J)
{
    //	double y0 = gsl_vector_get(x,0);
    //	double slope = gsl_vector_get(x,1);
        double width = gsl_vector_get(x,2);

        int numPeaks = (x->size-3)/2;

        QVector<double> ampList,centerList;
        ampList.reserve(numPeaks);
        centerList.reserve(numPeaks);
        for(int i=0; i<numPeaks; i++)
        {
            ampList.append(gsl_vector_get(x,3+2*i));
            centerList.append(gsl_vector_get(x,4+2*i));
        }

        QVector<QPointF> *d = static_cast<QVector<QPointF> *>(data);

        for(int i=0; i<d->size(); i++)
        {
            double xVal = d->at(i).x();
            double dy0 = 1.0;
            double dSlope = xVal;
            double dWidth = 0.0;

            for(int j=0; j<numPeaks; j++)
            {
                double A = ampList.at(j);
                double x0 = centerList.at(j);
                double G = gauss(xVal,x0,width);
                double expArg = twoLogTwo*(xVal-x0)*(xVal-x0)/(width*width);

                double dX0i = 2.0*A*G*twoLogTwo*(xVal-x0)/(width*width);
                double dAi = G;
                dWidth += 2.0*A*G*expArg/width;

                gsl_matrix_set(J,i,3+2*j,dAi);
                gsl_matrix_set(J,i,4+2*j,dX0i);
            }

            gsl_matrix_set(J,i,0,dy0);
            gsl_matrix_set(J,i,1,dSlope);
            gsl_matrix_set(J,i,2,dWidth);

        }

        return GSL_SUCCESS;
}

int Analysis::gaussSingle_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
    gaussSingle_f(x,data,f);
    gaussSingle_df(x,data,J);

    return GSL_SUCCESS;
}

double Analysis::kahanSum(const QVector<double> dat)
{
    double sum = 0.0;
    double c = 0.0;
    for(int i=0; i<dat.size(); i++)
    {
        double y = dat.at(i) - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}
