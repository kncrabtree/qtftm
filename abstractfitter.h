#ifndef ABSTRACTFITTER_H
#define ABSTRACTFITTER_H

#include <QObject>

#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_blas.h>
#include <eigen3/Eigen/Core>

#include "fitresult.h"
#include "scan.h"
#include "ftworker.h"


class AbstractFitter : public QObject
{
	Q_OBJECT
public:
    AbstractFitter(const FitResult::FitterType t, QObject *parent = nullptr);
    virtual ~AbstractFitter();

    FitResult::FitterType type() const { return d_type; }
    QPair<QVector<QPointF>, double> doStandardFT(const Fid fid);

    void setDelay(double d) { ftw.setDelay(d); }
    void setHpf(double d) { ftw.setHpf(d); }
    void setExp(double d) { ftw.setExp(d); }
    void setAutoPad(bool b) { ftw.setAutoPad(b); }
    void setRemoveDC(bool b) { ftw.setRemoveDC(b); }
    void setUseWindow(bool b);
    void setFidSaturationLimit(double d) { d_fidSaturationLimit = d; }

    double delay() const { return ftw.delay(); }
    double hpf() const { return ftw.hpf(); }
    double exp() const { return ftw.exp(); }
    bool autoPad() const { return ftw.autoPad(); }
    bool removeDC() const { return ftw.removeDC(); }
    bool isUseWindow() const { return ftw.isUseWindow(); }
    double fidSaturationLimit() const { return d_fidSaturationLimit; }

    void setBufferGas(const FitResult::BufferGas &bg) { d_bufferGas = bg; }
    void setTemperature(const double t) { d_temperature = t; }

    const FitResult::BufferGas& bufferGas() const { return d_bufferGas; }
    double temperature() const { return d_temperature; }


protected:
    struct NlOptFitData {
        FitResult::FitterType type;
        FitResult::LineShape lsf;
        int numSingle;
        int numPairs;
        QVector<QPointF> ft;
        int numEvals;
        QVector<double> lastParams;
        QVector<double> lastJacobian;
        gsl_matrix *J;
    };

    virtual void calcCoefs(int winSize, int polyOrder);
    virtual QList<QPair<QPointF, double> > findPeaks(QVector<QPointF> ft, double noisey0, double noisem, double minSNR);
    virtual FitResult dopplerFit(const QVector<QPointF> ft, const FitResult &in, const QList<double> commonParams, const QList<FitResult::DopplerPairParameters> dpParams, const QList<QPointF> singleParams, const int maxIterations);
    static double lor(double x, double center, double fwhm);
    static double gauss(double x, double center, double fwhm);
    static double nlOptFitFunction(const std::vector<double> &p, std::vector<double> &grad, void *fitData);

    const FitResult::FitterType d_type;
    FtWorker ftw;
    FitResult::BufferGas d_bufferGas;
    double d_temperature;
    Eigen::MatrixXd d_coefs;
    int d_window;
    int d_polyOrder;
    double d_fidSaturationLimit;

public slots:
    virtual FitResult doFit(const Scan s) =0;
    bool isFidSaturated(const Scan s);
    bool isFidSaturated(const Fid f);

signals:
    void fitComplete(const FitResult &);

private:
    static constexpr double twoLogTwo = 2.0*log(2.0);
    static constexpr double rootTwoLogTwo = sqrt(twoLogTwo);
};

#endif // ABSTRACTFITTER_H
