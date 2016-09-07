#include "analysis.h"
#include <math.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_sf.h>
#include "fitresult.h"


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
