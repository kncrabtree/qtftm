#include "fitresult.h"
#include <QSharedData>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <math.h>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "analysis.h"
#include <QSettings>
#include <QApplication>

class FitData : public QSharedData
{
public:

    FitData(FitResult::FitterType t = FitResult::NoFitting, FitResult::FitCategory c = FitResult::Invalid, FitResult::LineShape s = FitResult::Lorentzian) :
        type(t), category(c), lineShape(s), gslStatusCode(0), iterations(0), chisq(0.0),
        offsetFreq(0.0), numParams(0), delay(0.0), hpf(0.0), exp(0.0), rdc(true), zpf(false), window(false), baselineY0Slope(qMakePair(0.0,0.0)), scanNum(-1),
        bufferGas(FitResultBG::bufferNe), temperature(293.15), log(QString()) {}
    FitData(const FitData &other) : QSharedData(other), type(other.type), category(other.category), lineShape(other.lineShape),
		gslStatusCode(other.gslStatusCode), gslStatusMessage(other.gslStatusMessage), iterations(other.iterations),
		chisq(other.chisq),	offsetFreq(other.offsetFreq), numParams(other.numParams),
        delay(other.delay), hpf(other.hpf), exp(other.exp), rdc(other.rdc), zpf(other.zpf), window(other.window),
		allFitParameters(other.allFitParameters), allFitUncertainties(other.allFitUncertainties),
		freqAmpPairList(other.freqAmpPairList),	freqAmpUncPairList(other.freqAmpUncPairList),
		freqAmpSingleList(other.freqAmpSingleList), freqAmpUncSingleList(other.freqAmpUncSingleList),
		baselineY0Slope(other.baselineY0Slope), scanNum(other.scanNum), bufferGas(other.bufferGas),
		temperature(other.temperature), log(other.log) {}
	~FitData(){}

	FitResult::FitterType type;
	FitResult::FitCategory category;
    FitResult::LineShape lineShape;
	int gslStatusCode;
	QString gslStatusMessage;
	int iterations;
	double chisq;
	double offsetFreq;
	int numParams;
	double delay;
	double hpf;
	double exp;
    bool rdc;
    bool zpf;
    bool window;
	QList<double> allFitParameters;
	QList<double> allFitUncertainties;
	QList<QPair<double,double> > freqAmpPairList;
	QList<QPair<double,double> > freqAmpUncPairList;
	QList<QPair<double,double> > freqAmpSingleList;
	QList<QPair<double,double> > freqAmpUncSingleList;
	QPair<double,double> baselineY0Slope;
	int scanNum;
	FitResult::BufferGas bufferGas;
	double temperature;
	QString log;


};

void FitResult::deleteFitResult(const int num)
{
    if(num < 1)
        return;

    int dirMillionsNum = num/1000000;
    int dirThousandsNum = num/1000;

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/autofit/%1/%2").arg(dirMillionsNum).arg(dirThousandsNum));

    //open file
    QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(num));
    if(f.exists())
        f.remove();
}

FitResult::FitResult() : data(new FitData)
{
}

FitResult::FitResult(int scanNum) : data(new FitData)
{
	loadFromFile(scanNum);
}

FitResult::FitResult(const FitResult::FitterType t, const FitResult::FitCategory c) : data(new FitData(t,c))
{
}

FitResult::FitResult(const FitResult &other) : data(other.data)
{
}

FitResult &FitResult::operator=(const FitResult &other)
{
	if (this != &other)
		data.operator=(other.data);
	return *this;
}

FitResult::~FitResult()
{

}

FitResult::FitterType FitResult::type() const
{
	return data->type;
}

FitResult::FitCategory FitResult::category() const
{
    return data->category;
}

FitResult::LineShape FitResult::lineShape() const
{
    return data->lineShape;
}

int FitResult::status() const
{
	return data->gslStatusCode;
}

QString FitResult::statusMessage() const
{
	return data->gslStatusMessage;
}

int FitResult::iterations() const
{
	return data->iterations;
}

double FitResult::chisq() const
{
	return data->chisq;
}

double FitResult::probeFreq() const
{
	return data->offsetFreq;
}

int FitResult::numParams() const
{
	return data->numParams;
}

double FitResult::delay() const
{
	return data->delay;
}

double FitResult::hpf() const
{
	return data->hpf;
}

double FitResult::exp() const
{
    return data->exp;
}

bool FitResult::rdc() const
{
    return data->rdc;
}

bool FitResult::zpf() const
{
    return data->zpf;
}

bool FitResult::isUseWindow() const
{
    return data->window;
}

QList<double> FitResult::allFitParams() const
{
	return data->allFitParameters;
}

QList<double> FitResult::allFitUncertainties() const
{
	return data->allFitUncertainties;
}

QList<QPair<double, double> > FitResult::freqAmpPairList() const
{
	return data->freqAmpPairList;
}

QList<QPair<double, double> > FitResult::freqAmpPairUncList() const
{
	return data->freqAmpUncPairList;
}

QList<QPair<double, double> > FitResult::freqAmpSingleList() const
{
	return data->freqAmpSingleList;
}

QList<QPair<double, double> > FitResult::freqAmpSingleUncList() const
{
    return data->freqAmpUncSingleList;
}

FitResult::DopplerPairParameters FitResult::dopplerParameters(int index) const
{
    DopplerPairParameters out(0.0,0.0,0.0);

    if(type() == DopplerPair || type() == Mixed)
    {
        int offsetIndex = 4 + 3*index;
        out.amplitude = data->allFitParameters.value(offsetIndex);
        out.alpha = data->allFitParameters.value(offsetIndex+1);
        out.centerFreq = data->allFitParameters.value(offsetIndex+2);
    }

    return out;

}

QPair<double, double> FitResult::baselineY0Slope() const
{
    return data->baselineY0Slope;
}

double FitResult::width() const
{
    if(type() == RobustLinear)
        return 0.0;

    if(type() == Single)
        return data->allFitParameters.value(2);

    return data->allFitParameters.value(3);
}

double FitResult::splitting() const
{
    if(type() == DopplerPair || type() == Mixed)
        return data->allFitParameters.value(2);

    return 0.0;
}

QVector<QPointF> FitResult::toXY() const
{
	QVector<QPointF> out;
	out.reserve(1000);

	double width = 0.0, split = 0.0;
	int offsetIndex = 0;
	if(numParams() > 3)
	{
        if(type() == Single)
		{
			width = fabs(allFitParams().at(2));
			offsetIndex = 3;
		}
		else
		{
			width = fabs(allFitParams().at(3));
			split = allFitParams().at(2);
			offsetIndex = 4;
		}
	}

	for(int i=0; i<1001; i++)
	{
		double x = (double)i/1000.0;
		double y = baselineY0Slope().first + x*baselineY0Slope().second;

		for(int j=0; j<freqAmpPairList().size(); j++)
		{
			double A = allFitParams().at(offsetIndex+3*j);
			double al = allFitParams().at(offsetIndex+3*j+1);
			double x0 = allFitParams().at(offsetIndex+3*j+2);

            y += 2.0*A*(al*lsf(x,x0-split/2.0,width) + (1.0-al)*lsf(x,x0+split/2.0,width));
		}

		for(int j=0; j<freqAmpSingleList().size(); j++)
		{
			double A = allFitParams().at(offsetIndex+3*freqAmpPairList().size()+2*j);
			double x0 = allFitParams().at(offsetIndex+3*freqAmpPairList().size()+2*j+1);

            y += A*lsf(x,x0,width);
		}

		out.append(QPointF(probeFreq()+x,y));
	}

	return out;
}

FitResult::BufferGas FitResult::bufferGas() const
{
	return data->bufferGas;
}

double FitResult::temperature() const
{
	return data->temperature;
}

QString FitResult::log() const
{
    return data->log;
}

double FitResult::lsf(double x, double x0, double w) const
{
    if(lineShape() == Lorentzian)
        return Analysis::lor(x,x0,w);
    else if(lineShape() == Gaussian)
        return Analysis::gauss(x,x0,w);

    return 0.0;
}

QStringList FitResult::parameterList() const
{
	QStringList out;

	if(allFitParams().size() < 2)
		return out;

	out.append(QString::fromUtf8("y0\t%1 ± %2").arg(QString::number(allFitParams().at(0),'f',6))
			 .arg(QString::number(allFitUncertainties().at(0),'f',6)));
	out.append(QString::fromUtf8("m\t%1 ± %2").arg(QString::number(allFitParams().at(1),'f',6))
			 .arg(QString::number(allFitUncertainties().at(1),'f',6)));

	if(allFitParams().size() == 2)
		return out;

	if(freqAmpPairList().size()>0)
	{
		if(allFitParams().size() < 4+freqAmpPairList().size()*3)
			return out;

		out.append(QString::fromUtf8("spl\t%1 ± %2").arg(QString::number(allFitParams().at(2),'f',6))
				 .arg(QString::number(allFitUncertainties().at(2),'f',6)));
		out.append(QString::fromUtf8("wid\t%1 ± %2").arg(QString::number(allFitParams().at(3),'f',6))
				 .arg(QString::number(allFitUncertainties().at(3),'f',6)));

		for(int i=0;i<freqAmpPairList().size();i++)
		{
			out.append(QString::fromUtf8("pA%1\t%2 ± %3").arg(QString::number(i+1))
					 .arg(QString::number(allFitParams().at(4+3*i),'f',6))
					 .arg(QString::number(allFitUncertainties().at(4+3*i),'f',6)));
			out.append(QString::fromUtf8("pα%1\t%2 ± %3").arg(QString::number(i+1))
					 .arg(QString::number(allFitParams().at(5+3*i),'f',6))
					 .arg(QString::number(allFitUncertainties().at(5+3*i),'f',6)));
			out.append(QString::fromUtf8("px%1\t%2 ± %3").arg(QString::number(i+1))
					 .arg(QString::number(allFitParams().at(6+3*i)+probeFreq(),'f',6))
					 .arg(QString::number(allFitUncertainties().at(6+3*i),'f',6)));
		}

		if(allFitParams().size() < 4+freqAmpPairList().size()*3+freqAmpSingleList().size()*2)
			return out;

		int offset = 4+freqAmpPairList().size()*3;
		for(int i=0; i<freqAmpSingleList().size(); i++)
		{
			out.append(QString::fromUtf8("sA%1\t%2 ± %3").arg(QString::number(i+1))
					 .arg(QString::number(allFitParams().at(offset+2*i),'f',6))
					 .arg(QString::number(allFitUncertainties().at(offset+2*i),'f',6)));
			out.append(QString::fromUtf8("sx%1\t%2 ± %3").arg(QString::number(i+1))
					 .arg(QString::number(allFitParams().at(offset+2*i+1)+probeFreq(),'f',6))
					 .arg(QString::number(allFitUncertainties().at(offset+2*i+1),'f',6)));
		}
	}
	else
	{
		out.append(QString::fromUtf8("wid\t%1 ± %2").arg(QString::number(allFitParams().at(2),'f',6))
				 .arg(QString::number(allFitUncertainties().at(2),'f',6)));

		int offset = 3;
		if(allFitParams().size() < offset + 2*freqAmpSingleList().size())
            return out;//"Martin, Marie-Aline" <mmartin@cfa.harvard.edu>

		for(int i=0; i<freqAmpSingleList().size(); i++)
		{
			out.append(QString::fromUtf8("sA%1\t%2 ± %3").arg(QString::number(i+1))
					 .arg(QString::number(allFitParams().at(offset+2*i),'f',6))
					 .arg(QString::number(allFitUncertainties().at(offset+2*i),'f',6)));
			out.append(QString::fromUtf8("sx%1\t%2 ± %3").arg(QString::number(i+1))
					 .arg(QString::number(allFitParams().at(offset+2*i+1)+probeFreq(),'f',6))
					 .arg(QString::number(allFitUncertainties().at(offset+2*i+1),'f',6)));
		}

	}

	return out;
}

void FitResult::setType(FitResult::FitterType t)
{
	data->type = t;
}

void FitResult::setCategory(FitResult::FitCategory c)
{
    data->category = c;
}

void FitResult::setLineShape(FitResult::LineShape s)
{
    data->lineShape = s;
}

void FitResult::setStatus(int s)
{
	data->gslStatusCode = s;
	data->gslStatusMessage = QString(gsl_strerror(s));
}

void FitResult::setIterations(int i)
{
	data->iterations = i;
}

void FitResult::setChisq(double c)
{
	data->chisq = c;
}

void FitResult::setProbeFreq(double p)
{
	data->offsetFreq = p;
}

void FitResult::setDelay(double d)
{
	data->delay = d;
}

void FitResult::setHpf(double h)
{
	data->hpf = h;
}

void FitResult::setExp(double e)
{
    data->exp = e;
}

void FitResult::setRdc(bool b)
{
    data->rdc = b;
}

void FitResult::setZpf(bool b)
{
    data->zpf = b;
}

void FitResult::setUseWindow(bool b)
{
    data->window = b;
}

void FitResult::setFitParameters(gsl_vector *c, gsl_matrix *covar, int numSingle)
{
	data->numParams = c->size;
	data->allFitParameters.clear();
	data->allFitUncertainties.clear();
	if(data->numParams < 2)
		return;
	double uncFactor = GSL_MAX_DBL(1.0, sqrt(data->chisq));
	for(int i=0; i<data->numParams; i++)
	{
		data->allFitParameters.append(gsl_vector_get(c,i));
		data->allFitUncertainties.append(uncFactor*gsl_matrix_get(covar,i,i));
	}

	data->baselineY0Slope.first = data->allFitParameters.at(0);
	data->baselineY0Slope.second = data->allFitParameters.at(1);


	data->freqAmpPairList.clear();
	data->freqAmpSingleList.clear();
	data->freqAmpUncPairList.clear();
	data->freqAmpUncSingleList.clear();

    if(type() == DopplerPair)
	{
		data->allFitParameters[3] = fabs(data->allFitParameters.at(3));
		for(int i=4; i<data->numParams; i+=3)
		{
			data->freqAmpPairList.append(qMakePair(data->allFitParameters.at(i+2)+data->offsetFreq,
										    data->allFitParameters.at(i)));
			data->freqAmpUncPairList.append(qMakePair(data->allFitUncertainties.at(i+2),
										    data->allFitUncertainties.at(i)));
		}
	}

    if(type() == Mixed)
	{
		data->allFitParameters[3] = fabs(data->allFitParameters.at(3));
		for(int i=4; i<data->numParams - 2*numSingle; i+=3)
		{
			data->freqAmpPairList.append(qMakePair(data->allFitParameters.at(i+2)+data->offsetFreq,
										    data->allFitParameters.at(i)));
			data->freqAmpUncPairList.append(qMakePair(data->allFitUncertainties.at(i+2),
										    data->allFitUncertainties.at(i)));
		}

		for(int i=data->numParams-2*numSingle; i < data->numParams; i+=2)
		{
			data->freqAmpSingleList.append(qMakePair(data->allFitParameters.at(i+1)+data->offsetFreq,
										    data->allFitParameters.at(i)));
			data->freqAmpUncSingleList.append(qMakePair(data->allFitUncertainties.at(i+1),
										    data->allFitUncertainties.at(i)));
		}
	}

    if(type() == Single)
	{
		data->allFitParameters[2] = fabs(data->allFitParameters.at(2));
		for(int i=3; i<data->numParams; i+=2)
		{
			data->freqAmpSingleList.append(qMakePair(data->allFitParameters.at(i+1)+data->offsetFreq,
										    data->allFitParameters.at(i)));
			data->freqAmpUncSingleList.append(qMakePair(data->allFitUncertainties.at(i+1),
										    data->allFitUncertainties.at(i)));
		}
	}

}

void FitResult::setFitParameters(QList<double> params, QList<double> uncs, int numPairs, int numSingle)
{
	data->allFitParameters = params;
	data->allFitUncertainties = uncs;
    data->numParams = params.size();
    if(data->numParams < 2 || uncs.size() != data->numParams)
        return;


	data->freqAmpPairList.clear();
	data->freqAmpSingleList.clear();
	data->freqAmpUncPairList.clear();
	data->freqAmpUncSingleList.clear();

    int offsetIndex = 4;
    if(type() == Single)
        offsetIndex = 3;

    data->baselineY0Slope.first = data->allFitParameters.at(0);
    data->baselineY0Slope.second = data->allFitParameters.at(1);



    for(int i=0; i<numPairs; i++)
    {
        data->freqAmpPairList.append(qMakePair(params.at(offsetIndex+3*i+2)+data->offsetFreq,
                                               params.at(offsetIndex+3*i)));
        data->freqAmpUncPairList.append(qMakePair(uncs.at(offsetIndex+3*i+2),
                                                  uncs.at(offsetIndex+3*i)));
    }
    for(int i=0; i<numSingle; i++)
    {
        data->freqAmpSingleList.append(qMakePair(params.at(offsetIndex+3*numPairs+2*i+1)+data->offsetFreq,
                                                 params.at(offsetIndex+3*numPairs+2*i)));
        data->freqAmpUncPairList.append(qMakePair(uncs.at(offsetIndex+3*numPairs+2*i+1)+data->offsetFreq,
                                                  uncs.at(offsetIndex+3*numPairs+2*i)));
    }
}

void FitResult::setBaselineY0Slope(double y0, double slope)
{
	data->baselineY0Slope = qMakePair(y0,slope);
}

void FitResult::setBufferGas(BufferGas bg)
{
	data->bufferGas = bg;
}

void FitResult::setBufferGas(QString bgName)
{
	//default buffer gas is Ne. Check to see if the bgname contains something different
	if(bgName.contains(QString("H2"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferH2;
	else if(bgName.contains(QString("He"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferHe;
	else if(bgName.contains(QString("N2"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferN2;
	else if(bgName.contains(QString("O2"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferO2;
	else if(bgName.contains(QString("Ne"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferNe;
	else if(bgName.contains(QString("Ar"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferAr;
	else if(bgName.contains(QString("Kr"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferKr;
	else if(bgName.contains(QString("Xe"),Qt::CaseSensitive))
        data->bufferGas = FitResultBG::bufferXe;
	else
        data->bufferGas = FitResultBG::bufferNe;
}

void FitResult::setTemperature(double t)
{
	data->temperature = t;
}

void FitResult::appendToLog(const QString s)
{
    data->log.append(QString("%1\n").arg(s));
}

void FitResult::setLogText(const QString s)
{
    data->log = s;
}

void FitResult::save(int num)
{
	if(num < 1)
		return;

	data->scanNum = num;

	int dirMillionsNum = (int)floor((double) num/1000000.0);
	int dirThousandsNum = (int)floor((double) num/1000.0);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QString savePath = s.value(QString("savePath"),QString(".")).toString();

	QDir d(savePath + QString("/autofit/%1/%2").arg(dirMillionsNum).arg(dirThousandsNum));
	if(!d.exists())
	{
		if(!d.mkpath(d.absolutePath()))
			return;
	}

	//create output file
	QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(num));

	if(!f.open(QIODevice::WriteOnly))
		return;

	QTextStream t(&f);
	t.setRealNumberNotation(QTextStream::ScientificNotation);
	t.setRealNumberPrecision(12);

	QString tab("\t");
	QString nl("\n");

    t << QString("#FitType") << tab << (int)type() << nl;
	t << QString("#Category") << tab << (int)category() << nl;
    t << QString("#Lineshape") << tab << (int)lineShape() << nl;
	t << QString("#Status") << tab << status() << nl;
	t << QString("#Iterations") << tab << iterations() << nl;
	t << QString("#Chisq") << tab << chisq() << nl;
	t << QString("#ProbeFreq") << tab << probeFreq() << nl;
	t << QString("#NumParams") << tab << numParams() << nl;
	t << QString("#Delay") << tab << delay() << nl;
	t << QString("#HighPass") << tab << hpf() << nl;
	t << QString("#Exp") << tab << exp() << nl;
    t << QString("#RemoveDC") << tab << rdc() << nl;
    t << QString("#ZeroPad") << tab << zpf() << nl;
    t << QString("#UseWindow") << tab << isUseWindow() << nl;
	t << QString("#SinglePeaks") << tab << freqAmpSingleList().size() << nl;
	t << QString("#BufferGas") << tab << bufferGas().name << nl;
	t << QString("#Temperature") << tab << temperature() << nl;
	t << QString("#ParameterList") << tab;
	for(int i=0;i<allFitParams().size() && i<allFitUncertainties().size(); i++)
		t << nl << allFitParams().at(i) << tab << allFitUncertainties().at(i);

	t << nl << nl << QString("[log]") << nl;
	t << log();

	t.flush();
	f.close();
}

void FitResult::loadFromFile(int num)
{
	if(num < 1)
		return;

	int dirMillionsNum = (int)floor((double) num/1000000.0);
	int dirThousandsNum = (int)floor((double) num/1000.0);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	QString savePath = s.value(QString("savePath"),QString(".")).toString();
	QDir d(savePath + QString("/autofit/%1/%2").arg(dirMillionsNum).arg(dirThousandsNum));

	//open file
	QFile f(QString("%1/%2.txt").arg(d.absolutePath()).arg(num));

	if(!f.open(QIODevice::ReadOnly))
		return;

	data->scanNum = num;
	int numSingle = 0;

    //for backwards compatibility, set lineshape to Lorentzian
    setLineShape(Lorentzian);

	while(!f.atEnd())
	{
		QString line = QString(f.readLine());

		if(line.startsWith(QString("#ParameterList")))
			break;

		QStringList l = line.split(QString("\t"));
		if(l.size()<2)
			continue;

		QString key = l.at(0).trimmed();
		QString value = l.at(1).trimmed();

		if(key.startsWith(QString("#Type")))
        {
            DeprecatedFitterType t = (DeprecatedFitterType)value.toInt();
            if(t == Dep_LorentzianDopplerPairLM || t == Dep_LorentzianDopplerPairLMS)
                setType(DopplerPair);
            else if(t == Dep_NoFitting)
                setType(NoFitting);
            else if(t == Dep_RobustLinear)
                setType(RobustLinear);
            else if(t == Dep_LorentzianMixedLM || t == Dep_LorentzianMixedLMS)
                setType(Mixed);
            else if(t == Dep_LorentzianSingleLM || t == Dep_LorentzianSingleLMS)
                setType(Single);
        }
        else if(key.startsWith(QString("#FitType")))
            setType((FitterType)value.toInt());
		else if(key.startsWith(QString("#Category")))
			setCategory((FitCategory)value.toInt());
        else if(key.startsWith(QString("#Lineshape")))
        {
            LineShape lsf = (LineShape)value.toInt();
            setLineShape(lsf);
            //for backwards compatibility
            if(lsf == Lorentzian)
                setUseWindow(false);
            else if(lsf == Gaussian)
                setUseWindow(true);
        }
		else if(key.startsWith(QString("#Status")))
			setStatus(value.toInt());
		else if(key.startsWith(QString("#Iterations")))
			setIterations(value.toInt());
		else if(key.startsWith(QString("#Chisq")))
			setChisq(value.toDouble());
		else if(key.startsWith(QString("#ProbeFreq")))
			setProbeFreq(value.toDouble());
		else if(key.startsWith(QString("#NumParams")))
			data->numParams = value.toInt();
		else if(key.startsWith(QString("#Delay")))
			setDelay(value.toDouble());
		else if(key.startsWith(QString("#HighPass")))
			setHpf(value.toDouble());
		else if(key.startsWith(QString("#Exp")))
			setExp(value.toDouble());
        else if(key.startsWith(QString("#RemoveDC")))
            setRdc((bool)value.toInt());
        else if(key.startsWith(QString("#ZeroPad")))
            setZpf((bool)value.toInt());
        else if(key.startsWith(QString("#UseWindow")))
            setUseWindow((bool)value.toInt());
		else if(key.startsWith(QString("#BufferGas")))
			setBufferGas(value);
		else if(key.startsWith(QString("#Temperature")))
			setTemperature(value.toDouble());
		else if(key.startsWith(QString("#SinglePeaks")))
			numSingle = value.toInt();
	}

	QList<double> p, u;
	while(!f.atEnd())
	{
		QString line = QString(f.readLine());
		if(line.isEmpty())
			continue;

		if(line.startsWith(QString("[log]")))
			break;

		QStringList l = line.split(QString("\t"));
		if(l.size()<2)
			continue;

		p.append(l.at(0).trimmed().toDouble());
		u.append(l.at(1).trimmed().toDouble());
	}

    int numPairs;
    if(type() == Single)
        numPairs = 0;
    else
        numPairs = (p.size()-4-2*numSingle)/3;
    setFitParameters(p,u,numPairs,numSingle);

	appendToLog(f.readAll());

	f.close();

}
