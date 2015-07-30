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
		BufferGas(QString n, double m, double g) :
			name(n), mass(m*GSL_CONST_CGS_MASS_PROTON), gamma(g) {}
	};

	enum FitterType {
		NoFitting,
		RobustLinear,
		LorentzianDopplerPairLMS,
		LorentzianDopplerPairLM,
		LorentzianMixedLMS,
		LorentzianMixedLM,
		LorentzianSingleLMS,
		LorentzianSingleLM
	};

	enum FitCategory {
	    Invalid,
	    NoPeaksFound,
	    Saturated,
	    Success
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
	int status() const;
	QString statusMessage() const;
	int iterations() const;
	double chisq() const;
	double probeFreq() const;
	int numParams() const;
	double delay() const;
	double hpf() const;
	double exp() const;
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

	QStringList parameterList() const;

	void setType(FitterType t);
	void setCategory(FitCategory c);
	void setStatus(int s);
	void setIterations(int i);
	void setChisq(double c);
	void setProbeFreq(double p);
	void setDelay(double d);
	void setHpf(double h);
	void setExp(double e);
	void setFitParameters(gsl_vector *c, gsl_matrix *covar, int numSingle = 0);
	void setFitParameters(QList<double> params, QList<double> uncs, int numSingle = 0);
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

#endif // FITRESULT_H
