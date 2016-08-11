#include "ftworker.h"
#include <gsl/gsl_const.h>
#include <gsl/gsl_sf.h>
#include "analysis.h"

FtWorker::FtWorker(QObject *parent) :
	QObject(parent), real(nullptr), work(nullptr), d_numPnts(0),
	realPadded(nullptr), workPadded(nullptr), d_numPntsPadded(0),
    d_delay(0.0), d_hpf(0.0), d_exp(0.0), d_autoPadFids(false), d_removeDC(true), d_lastMax(0.0), d_useWindow(true)
{
}

FtWorker::~FtWorker()
{
	if(real)
		gsl_fft_real_wavetable_free(real);
	if(work)
		gsl_fft_real_workspace_free(work);
	if(realPadded)
		gsl_fft_real_wavetable_free(realPadded);
	if(workPadded)
		gsl_fft_real_workspace_free(workPadded);

}

QPair<QVector<QPointF>, double> FtWorker::doFT(const Fid f)
{
    if(f.spacing() < 1e-20 || f.size() == 0)
    {
	   QPair<QVector<QPointF>, double> out = qMakePair(QVector<QPointF>(201),0.0);
	   emit ftDone(out.first,out.second);
	   return out;
    }

    int startSize = f.size();

    //first, apply any filtering that needs to be done
    Fid fid = filterFid(f);
    if(d_autoPadFids)
        fid = padFid(fid);

    //make a vector of points for display purposes
    QVector<QPointF> displayFid;
    QVector<double> data = fid.toVector();
    displayFid.reserve(data.size());
    for(int i=0; i<data.size(); i++)
        displayFid.append(QPointF((double)i*fid.spacing()*1.0e6,data.at(i)));

    emit fidDone(displayFid);

    //might need to allocate or reallocate workspace and wavetable
    gsl_fft_real_wavetable *theTable;
    gsl_fft_real_workspace *theWorkspace;
    if(d_autoPadFids)
    {
	    if(fid.size() != d_numPntsPadded)
	    {
		    d_numPntsPadded = fid.size();

		    if(realPadded)
		    {
			    gsl_fft_real_wavetable_free(realPadded);
			    gsl_fft_real_workspace_free(workPadded);
		    }

		    realPadded = gsl_fft_real_wavetable_alloc(d_numPntsPadded);
		    workPadded = gsl_fft_real_workspace_alloc(d_numPntsPadded);
	    }
	    theTable = realPadded;
	    theWorkspace = workPadded;
    }
    else
    {
	    if(fid.size() != d_numPnts)
	    {
		    d_numPnts = fid.size();

		    //free memory if this is a reallocation
		    if(real)
		    {
			    gsl_fft_real_wavetable_free(real);
			    gsl_fft_real_workspace_free(work);
		    }

		    real = gsl_fft_real_wavetable_alloc(d_numPnts);
		    work = gsl_fft_real_workspace_alloc(d_numPnts);
	    }
	    theTable = real;
	    theWorkspace = work;
	}

	QVector<QPointF> spectrum = calculateFT(fid,startSize,theTable,theWorkspace);
	return qMakePair(spectrum,d_lastMax);
}

QVector<QPointF> FtWorker::doFT_noPad(const Fid fid, bool offsetOnly)
{
	if(fid.size() < 2)
		return QVector<QPointF>();

	if(fid.size() != d_numPnts)
	{
		d_numPnts = fid.size();

		//free memory if this is a reallocation
		if(real)
		{
			gsl_fft_real_wavetable_free(real);
			gsl_fft_real_workspace_free(work);
		}

		real = gsl_fft_real_wavetable_alloc(d_numPnts);
		work = gsl_fft_real_workspace_alloc(d_numPnts);
	}

//	bool wasPadded = d_autoPadFids;
//	d_autoPadFids = false;
	Fid f = filterFid(fid);
//	d_autoPadFids = wasPadded;

	return calculateFT(f,fid.size(),real,work,offsetOnly);
}

QVector<QPointF> FtWorker::doFT_pad(const Fid fid, bool offsetOnly)
{
	if(fid.size() < 2)
		return QVector<QPointF>();

//	QVector<double> data = fid.toVector();
//	data.resize(Analysis::power2Nplus1(data.size()));
    Fid f = filterFid(fid);
    f = padFid(f);

	if(f.size() != d_numPntsPadded)
	{
		d_numPntsPadded = f.size();

		if(realPadded)
		{
			gsl_fft_real_wavetable_free(realPadded);
			gsl_fft_real_workspace_free(workPadded);
		}

		realPadded = gsl_fft_real_wavetable_alloc(d_numPntsPadded);
		workPadded = gsl_fft_real_workspace_alloc(d_numPntsPadded);
	}

//	bool wasPadded = d_autoPadFids;
//	d_autoPadFids = false;
//	Fid f2 = filterFid(f);
//	d_autoPadFids = wasPadded;

    return calculateFT(f,fid.size(),realPadded,workPadded, offsetOnly);
}

QVector<QPointF> FtWorker::calculateFT(const Fid fid, int realPoints, gsl_fft_real_wavetable *wt, gsl_fft_real_workspace *ws, bool offsetOnly)
{
	//prepare storage
	QVector<double> fftData(fid.toVector());
	QVector<QPointF> spectrum;
	spectrum.reserve(realPoints/2+1);
	double spacing = fid.spacing();
	double probe = fid.probeFreq();
	if(offsetOnly)
		probe = 0.0;

	//do the FT. See GNU Scientific Library documentation for details
	gsl_fft_real_transform (fftData.data(), 1, fftData.size(), wt, ws);

	//convert fourier coefficients into magnitudes. the coefficients are stored in half-complex format
	//see http://www.gnu.org/software/gsl/manual/html_node/Mixed_002dradix-FFT-routines-for-real-data.html
    //first point is DC; block it!
	spectrum << QPointF(probe,0.0);
	d_lastMax = 0.0;
	int i;
	for(i=1; i<fftData.size()-i; i++)
	{
		//calculate x value
		double x1 = probe + (double)i/(double)fftData.size()/spacing*1.0e-6;

		//calculate real and imaginary coefficients
		double coef_real = fftData.at(2*i-1);
		double coef_imag = fftData.at(2*i);

		//calculate magnitude and update max
		//note: Normalize output, and convert to mV
		double coef_mag = sqrt(coef_real*coef_real + coef_imag*coef_imag)/(double)realPoints*1000.0;
		d_lastMax = qMax(d_lastMax,coef_mag);

		spectrum.append(QPointF(x1,coef_mag));
	}
	if(i==d_numPnts-i)
	{
		double coef_mag = sqrt(fftData.at(fftData.size()-1)*fftData.at(fftData.size()-1))/(double)realPoints*1000.0;
		d_lastMax = qMax(d_lastMax,coef_mag);
		spectrum.append(QPointF(probe + (double)i/(double)d_numPnts/spacing*1.0e-6,coef_mag));
	}

	emit ftDone(spectrum, d_lastMax);
	return spectrum;
}

Fid FtWorker::filterFid(const Fid f)
{
	Fid fid;
	if(d_removeDC)
		fid = Analysis::removeDC(f);
	else
		fid = f;

	QVector<double> data;

	if(d_hpf > 0.0)
	{
		//apply a simple first-order high-pass filter
		//recall that spacing is in seconds, and d_hpf is in kHz
		data.reserve(fid.size());

		//rc is the time constant of the filter, and alpha is a coefficient used in the filter
		double rc = 0.5/d_hpf/M_PI/1000.0;
		double alpha = rc/(rc+fid.spacing());

		//TODO: consider using more advanced, higher-order filter?
		//algorithm is out[i] = alpha*out[i-1] + alpha*(in[i]-in[i-1])
		data.append(fid.at(0));
		for(int i=1; i<fid.size();i++)
			data.append(alpha*(data.at(i-1)+fid.at(i)-fid.at(i-1)));
	}
	else //no filtering, so set data equal to the fid
		data = QVector<double>(fid.toVector());

	if(d_delay>0.0)
	{
		//delay is in microseconds, fid is in seconds
		int i=0;
		while((double)i*fid.spacing() < d_delay*1.0e-6 && i < data.size())
		{
			data[i] = 0.0;
			i++;
		}
	}

	if(d_exp>0.0)
	{
		//multiply FID at each point with exponential decay
		for(int i=0; i<data.size(); i++)
			data[i]*=gsl_sf_exp(-(double)i*fid.spacing()/d_exp*1.0e6);
	}

    if(d_useWindow)
    {
        makeWinf(data.size());
        for(int i=0; i<data.size(); i++)
            data[i]*=d_winf[i];
    }

	//for synchronous use (eg the doFT function), return an FID object
    return Fid(fid.spacing(),fid.probeFreq(),data);
}

Fid FtWorker::padFid(const Fid f)
{
    QVector<double> data = f.toVector();

    //find second power of 2 larger than data size (eg 400 -> 1024; 200->512, 1000->2048, etc...)
    int padSize = Analysis::power2Nplus1(data.size());
    data.resize(padSize); //the resize function populates new entries with default-constructed value (0.0 for double)

    return Fid(f.spacing(),f.probeFreq(),data);
}

void FtWorker::makeWinf(int n)
{
    if(d_winf.size() == n)
        return;

    d_winf.resize(n);

    double N = static_cast<double>(n);
    double p2n = 2.0*M_PI/N;
    double p4n = 4.0*M_PI/N;
    double p6n = 6.0*M_PI/N;
    for(int i=0; i<n; i++)
    {
        double I = static_cast<double>(i);
        d_winf[i] = 0.35875 - 0.48829*cos(p2n*I) + 0.14128*cos(p4n*I) - 0.01168*cos(p6n*I);
    }
}
