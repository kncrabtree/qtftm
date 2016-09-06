#ifndef FITRESULT_H
#define FITRESULT_H

#include <QSharedDataPointer>
#include <QVector>
#include <QPointF>
#include <QPair>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_const_cgs.h>
#include <QString>

class FitData;

class FitResult
{
public:
	struct BufferGas {
		QString name;
		double mass;
		double gamma;

		BufferGas() : mass(1.0), gamma(1.6) {}
        BufferGas(const QString n, const double m, const double g) :
			name(n), mass(m*GSL_CONST_CGS_MASS_PROTON), gamma(g) {}

	};

    struct DopplerPairParameters {
        double amplitude;
        double alpha;
        double centerFreq;

        DopplerPairParameters(double a, double al, double c) :
            amplitude(a), alpha(al), centerFreq(c) {}
    };

	enum FitterType {
		NoFitting,
		RobustLinear,
        DopplerPair,
        Mixed,
        Single,
	};

    enum DeprecatedFitterType {
        Dep_NoFitting,
        Dep_RobustLinear,
        Dep_LorentzianDopplerPairLMS,
        Dep_LorentzianDopplerPairLM,
        Dep_LorentzianMixedLMS,
        Dep_LorentzianMixedLM,
        Dep_LorentzianSingleLMS,
        Dep_LorentzianSingleLM
    };

	enum FitCategory {
	    Invalid,
	    NoPeaksFound,
	    Saturated,
        Success,
        Fail
	};

    enum LineShape {
        Lorentzian,
        Gaussian
    };

    static void deleteFitResult(const int num);

	FitResult();
	FitResult(int scanNum);
	FitResult(const FitterType t, const FitCategory c);
	FitResult(const FitResult &other);
	FitResult &operator=(const FitResult &other);
	~FitResult();

	FitterType type() const;
	FitCategory category() const;
    LineShape lineShape() const;
	int status() const;
	QString statusMessage() const;
	int iterations() const;
	double chisq() const;
	double probeFreq() const;
	int numParams() const;
	double delay() const;
	double hpf() const;
	double exp() const;
    bool rdc() const;
    bool zpf() const;
    bool isUseWindow() const;
	QList<double> allFitParams() const;
	QList<double> allFitUncertainties() const;
	QList<QPair<double,double> > freqAmpPairList() const;
	QList<QPair<double,double> > freqAmpPairUncList() const;
	QList<QPair<double,double> > freqAmpSingleList() const;
	QList<QPair<double,double> > freqAmpSingleUncList() const;
	QPair<double,double> baselineY0Slope() const;
	QVector<QPointF> toXY() const;
	BufferGas bufferGas() const;
	double temperature() const;
	QString log() const;
    double lsf(double x, double x0, double w) const;

	QStringList parameterList() const;

	void setType(FitterType t);
	void setCategory(FitCategory c);
    void setLineShape(LineShape s);
	void setStatus(int s);
	void setIterations(int i);
	void setChisq(double c);
	void setProbeFreq(double p);
	void setDelay(double d);
	void setHpf(double h);
	void setExp(double e);
    void setRdc(bool b);
    void setZpf(bool b);
    void setUseWindow(bool b);
	void setFitParameters(gsl_vector *c, gsl_matrix *covar, int numSingle = 0);
    void setFitParameters(QList<double> params, QList<double> uncs, int numPairs, int numSingle);
	void setBaselineY0Slope(double y0, double slope);
	void setBufferGas(BufferGas bg);
	void setBufferGas(QString bgName);
	void setTemperature(double t);
	void appendToLog(const QString s);
	void save(int num);
	void loadFromFile(int num);


private:
	QSharedDataPointer<FitData> data;

};

namespace FitResultBG {
const FitResult::BufferGas bufferH2 = FitResult::BufferGas(QString("H2"),2.0*1.00794,7.0/5.0);
const FitResult::BufferGas bufferHe = FitResult::BufferGas(QString("He"),4.002602,5.0/3.0);
const FitResult::BufferGas bufferN2 = FitResult::BufferGas(QString("N2"),2.0*14.0067,7.0/5.0);
const FitResult::BufferGas bufferO2 = FitResult::BufferGas(QString("O2"),2.0*15.9994,7.0/5.0);
const FitResult::BufferGas bufferNe = FitResult::BufferGas(QString("Ne"),20.1797,5.0/3.0);
const FitResult::BufferGas bufferAr = FitResult::BufferGas(QString("Ar"),39.948,5.0/3.0);
const FitResult::BufferGas bufferKr = FitResult::BufferGas(QString("Kr"),83.798,5.0/3.0);
const FitResult::BufferGas bufferXe = FitResult::BufferGas(QString("Xe"),131.293,5.0/3.0);
}

#endif // FITRESULT_H
