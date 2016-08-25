#ifndef ABSTRACTFITTER_H
#define ABSTRACTFITTER_H

#include <QObject>

#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_blas.h>
#include <eigen3/Eigen/Core>

#include "analysis.h"
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

    double delay() const { return ftw.delay(); }
    double hpf() const { return ftw.hpf(); }
    double exp() const { return ftw.exp(); }
    bool autoPad() const { return ftw.autoPad(); }
    bool removeDC() const { return ftw.removeDC(); }
    bool isUseWindow() const { return ftw.isUseWindow(); }

    void setBufferGas(const FitResult::BufferGas &bg) { d_bufferGas = bg; }
    void setTemperature(const double t) { d_temperature = t; }

    const FitResult::BufferGas& bufferGas() const { return d_bufferGas; }
    double temperature() const { return d_temperature; }


protected:
    virtual void calcCoefs(int winSize, int polyOrder);
    virtual QList<QPair<QPointF, double> > findPeaks(QVector<QPointF> ft, double noisey0, double noisem, double minSNR);

    const FitResult::FitterType d_type;
    FtWorker ftw;
    FitResult::BufferGas d_bufferGas;
    double d_temperature;
    Eigen::MatrixXd d_coefs;
    int d_window;
    int d_polyOrder;

public slots:
    virtual FitResult doFit(const Scan s) =0;
    bool isFidSaturated(const Scan s);

signals:
    void fitComplete(const FitResult &);
};

#endif // ABSTRACTFITTER_H
