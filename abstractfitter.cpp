#include "abstractfitter.h"

#include <gsl/gsl_sf.h>
#include <eigen3/Eigen/SVD>
#include <eigen3/Eigen/QR>
#include <nlopt.hpp>

#include "analysis.h"

AbstractFitter::AbstractFitter(const FitResult::FitterType t, QObject *parent) :
    QObject(parent), d_type(t), d_window(11), d_polyOrder(6),
    d_fidSaturationLimit(0.6), d_snrLimit(5.0)
{
    calcCoefs(d_window,d_polyOrder);
}

AbstractFitter::~AbstractFitter()
{
}

QPair<QVector<QPointF>, double> AbstractFitter::doStandardFT(const Fid fid)
{
    return ftw.doFT(fid);
}

void AbstractFitter::setUseWindow(bool b)
{
    ftw.setUseWindow(b);

    if(b)
        calcCoefs(11,6);
    else
        calcCoefs(31,4);
}

void AbstractFitter::calcCoefs(int winSize, int polyOrder)
{
    if(polyOrder < 2)
        return;

    if(!(winSize % 2))
        return;

    if(winSize < polyOrder + 1)
        return;

    d_window = winSize;
    d_polyOrder = polyOrder;

    Eigen::MatrixXd xm(d_polyOrder+1,d_window);
    Eigen::MatrixXd bm(d_polyOrder+1,d_polyOrder+1);

    for(int i=0; i<xm.rows(); i++)
    {
        for(int j=0; j<(int)xm.cols(); j++)
        {
            int z = j - (d_window/2);
            double val = pow((double)z,(double)i);
            xm(i,j) = val;
        }
    }

    for(int i=0; i<bm.rows(); i++)
    {
        for(int j=0; j<bm.cols(); j++)
            bm(i,j) = (i == j ? 1.0 : 0.0);
    }

    Eigen::JacobiSVD<Eigen::MatrixXd,Eigen::FullPivHouseholderQRPreconditioner> svd(xm, Eigen::ComputeFullU | Eigen::ComputeFullV);
    d_coefs = svd.solve(bm);

}

QList<QPair<QPointF,double>> AbstractFitter::findPeaks(QVector<QPointF> ft, double noisey0, double noisem)
{
    if(ft.size() < d_window)
    {
        return QList<QPair<QPointF,double>>();
    }

    //calculate smoothed second derivative
    QVector<double> smth(ft.size());
    QVector<double> yDat(ft.size());
    Eigen::VectorXd c = d_coefs.col(2);
    int halfWin = c.rows()/2;

    int startIndex = halfWin;
    int endIndex = ft.size()-halfWin;

    for(int i=startIndex; i<endIndex; i++)
    {
        //apply savitsky-golay smoothing, ignoring prefactor of 2/h^2 because we're only interested in local minima
        double val = 0.0;
        for(int j=0; j<c.rows(); j++)
            val += c(j)*ft.at(i+j-halfWin).y();
        smth[i] = val;
        yDat[i] = ft.at(i).y();
    }

    QList<QPair<QPointF,double>> out;
    for(int i = startIndex+2; i<endIndex-2; i++)
    {
        double noise = noisey0 + noisem*ft.at(i).x();
        double thisSNR = ft.at(i).y()/noise;
        if(thisSNR >= d_snrLimit)
        {
            //intensity is high enough; ID a peak by a minimum in 2nd derivative
            if(((smth.at(i-2) > smth.at(i-1)) && (smth.at(i-1) > smth.at(i)) && (smth.at(i) < smth.at(i+1))) ||
                    ((smth.at(i-1) > smth.at(i)) && (smth.at(i) < smth.at(i+1)) && (smth.at(i+1) < smth.at(i+2))) )
                out.append(qMakePair(ft.at(i),thisSNR));
        }
    }

    return out;
}

FitResult AbstractFitter::dopplerFit(const QVector<QPointF> ft, const FitResult &in, const QList<double> commonParams, const QList<FitResult::DopplerPairParameters> dpParams, const QList<QPointF> singleParams, const int maxIterations)
{
    //copy existing info into output
    FitResult out(in);

    NlOptFitData fd;
    fd.ft = ft;
    fd.lsf = out.lineShape();
    fd.numPairs = dpParams.size();
    fd.numSingle = singleParams.size();
    fd.numEvals = 0;

    if(singleParams.isEmpty())
    {
        fd.type = FitResult::DopplerPair;
        out.appendToLog(QString("Fitting to %1 Doppler pairs").arg(dpParams.size()));
    }
    else if(dpParams.isEmpty())
    {
        fd.type = FitResult::Single;
        out.appendToLog(QString("Fitting to %1 single peaks").arg(singleParams.size()));
    }
    else
    {
        fd.type = FitResult::Mixed;
        out.appendToLog(QString("Fitting to %1 Doppler pairs and %2 single peaks").arg(dpParams.size()).arg(singleParams.size()));
    }

    out.setType(fd.type);

    QVector<double> params, lb, ub;
    params << commonParams.at(0) << commonParams.at(1);
    lb << 0.0 << -HUGE_VAL;
    ub << HUGE_VAL << HUGE_VAL;
    if(!dpParams.isEmpty())
    {
        params << commonParams.at(2);
        lb << commonParams.at(2)/2.0;
        ub << commonParams.at(2)*1.5;
    }
    params << commonParams.at(3);
    lb << commonParams.at(3)/3.0;
    ub << commonParams.at(3)*5.0;
    for(int i=0; i<dpParams.size(); i++)
    {
        double A = dpParams.at(i).amplitude;
        double al = dpParams.at(i).alpha;
        double x0 = dpParams.at(i).centerFreq;

        params << A << al << x0;
        lb << A/3.0 << 0.15 << x0-0.05;
        ub << A*3.0 << 0.85 << x0+0.05;
    }
    for(int i=0; i<singleParams.size(); i++)
    {
        double A = singleParams.at(i).y();
        double x0 = singleParams.at(i).x();

        params << A << x0;
        lb << A/3.0 << x0-0.05;
        ub << A*3.0 << x0+0.05;
    }

    fd.J = gsl_matrix_alloc(ft.size(),params.size());

    nlopt::opt opt(nlopt::LD_MMA,params.size());
    opt.set_min_objective(&nlOptFitFunction,&fd);
    opt.set_lower_bounds(lb.toStdVector());
    opt.set_upper_bounds(ub.toStdVector());
    opt.set_ftol_rel(1e-4);
    opt.set_maxeval(maxIterations);

    std::vector<double> p = params.toStdVector();
    double sqErr;
    try
    {
        opt.optimize(p,sqErr);

        out.appendToLog(QString("Fitting stopped after %1 iterations.").arg(fd.numEvals));
        out.setStatus(0);
        int dof = ft.size() - params.size();
        out.setChisq(sqErr/static_cast<double>(dof));
        out.appendToLog(QString("Chi squared: %1").arg(out.chisq(),0,'e',4));
        out.setIterations(fd.numEvals);

        gsl_matrix *covar = gsl_matrix_alloc(params.size(),params.size());
        gsl_multifit_covar(fd.J,0.0,covar);
        QList<double> uncs;
        for(unsigned int i=0; i<covar->size1; i++)
        {
            for(unsigned int j=0; j<covar->size2; j++)
            {
                if(i==j)
                    uncs.append(sqrt(gsl_matrix_get(covar,i,j)*out.chisq()));
            }
        }
        gsl_matrix_free(covar);

        out.setFitParameters(fd.lastParams.toList(),uncs,fd.numPairs,fd.numSingle);
        bool ok = true;
        for(int i=0; i<fd.lastParams.size(); i++)
        {
            if(qFuzzyCompare(fd.lastParams.at(i),lb.at(i)) || qFuzzyCompare(fd.lastParams.at(i),ub.at(i)))
            {
                ok = false;
                out.appendToLog(QString("Parameter %1 went outside allowed range.").arg(i));
            }
        }
        if(!ok)
            out.setCategory(FitResult::Fail);
        else
            out.setCategory(FitResult::Success);

    }
    catch(const std::exception &e)
    {
        //record error in out
        out.setCategory(FitResult::Fail);
        out.setStatus(-1);

        out.appendToLog(QString("Exception thrown during fit: %1").arg(QString(e.what())));
    }

    gsl_matrix_free(fd.J);
    return out;


}

double AbstractFitter::lor(double x, double center, double fwhm)
{
    return 1.0/(1.0 + 4.0*(x-center)*(x-center)/(fwhm*fwhm));
}

double AbstractFitter::gauss(double x, double center, double fwhm)
{
    double x_ = x-center;
    double num = x_*x_;
    double den = fwhm*fwhm/twoLogTwo;

    double out = gsl_sf_exp(-num/den);
    if(isnan(out))
        out = 0.0;
    return out;
}

double AbstractFitter::nlOptFitFunction(const std::vector<double> &p, std::vector<double> &grad, void *fitData)
{
    //Goal: minimize squared error of residuals
    //This function needs to calculate the squared error
    //of the residuals as well as the gradient of
    //this squared error as a function of each fit parameter

    NlOptFitData *fd = static_cast<NlOptFitData*>(fitData);
    fd->numEvals++;
    fd->lastParams = QVector<double>::fromStdVector(p);
    //get common parameters
    double y0, slope, split, width;
    int offsetIndex;
    if(fd->type == FitResult::Single)
    {
        y0 = p.at(0);
        slope = p.at(1);
        split = 0.0;
        width = p.at(2);
        offsetIndex = 3;
    }
    else
    {
        y0 = p.at(0);
        slope = p.at(1);
        split = p.at(2);
        width = p.at(3);
        offsetIndex = 4;
    }

    QVector<double> residualSq(fd->ft.size());
    QList<QVector<double>> gradient;
    if(!grad.empty())
    {
        for(unsigned int i=0; i<p.size(); i++)
            gradient.append(QVector<double>(fd->ft.size()));
    }
    double (*lsf)(double,double,double);
    if(fd->lsf == FitResult::Lorentzian)
        lsf = &lor;
    else
        lsf = &gauss;

    for(int i=0; i<residualSq.size(); i++)
    {
        double xVal = fd->ft.at(i).x();
        double y = y0 + slope*xVal;
        for(int j=0;j<fd->numPairs;j++)
        {
            double A = p.at(offsetIndex+3*j);
            double al = p.at(offsetIndex+3*j+1);
            double x0 = p.at(offsetIndex+3*j+2);

            y += 2.0*A*(al*lsf(xVal,x0-split/2.0,width) + (1.0-al)*lsf(xVal,x0+split/2.0,width));
        }

        for(int j=0; j<fd->numSingle; j++)
        {
            double A = p.at(offsetIndex+3*fd->numPairs+2*j);
            double x0 = p.at(offsetIndex+3*fd->numPairs+2*j+1);

            y += A*lsf(xVal,x0,width);
        }

        double diff = y-fd->ft.at(i).y();
        double d2 = 2.0*diff;
        residualSq[i] = diff*diff;

        if(!grad.empty())
        {
            //calculate Jacobian... different for Lor and Gauss
            //Res = sum (f(x;y0...) - ft.y)^2
            //dRes/dp = sum 2*(f(x;y0...) - ft.y)*df/dp

            //df/dy0 = 1.0 for both
            //df/dslope = x for both
            gradient[0][i] = d2;
            gradient[1][i] = d2*xVal;

            gsl_matrix_set(fd->J,i,0,1.0);
            gsl_matrix_set(fd->J,i,1,xVal);

            double dWidth = 0.0, dSplit = 0.0;

            for(int j=0;j<fd->numPairs;j++)
            {
                double A = p.at(offsetIndex+3*j);
                double al = p.at(offsetIndex+3*j+1);
                double x0 = p.at(offsetIndex+3*j+2);

                double dAi = 0.0;
                double dAlphai = 0.0;
                double dX0i = 0.0;

                if(fd->lsf == FitResult::Lorentzian)
                {
                    double redLor = lor(xVal,x0-split/2.0,width);
                    double blueLor = lor(xVal,x0+split/2.0,width);

                    dSplit += -(8.0*A/(width*width))*(al*(xVal-(x0-split/2.0))*(redLor*redLor)
                              - (1.0-al)*(xVal-(x0+split/2.0))*(blueLor*blueLor));
                    dWidth += (16.0*A/(width*width*width))*(al*(xVal-(x0-split/2.0))*(xVal-(x0-split/2.0))*(redLor*redLor)
                              +(1.0-al)*(xVal-(x0+split/2.0))*(xVal-(x0+split/2.0))*(blueLor*blueLor));

                    dAi = 2.0*(al*redLor + (1.0-al)*blueLor);
                    dAlphai = 2.0*A*(redLor - blueLor);
                    dX0i = (16.0*A/(width*width))*(al*(xVal-(x0-split/2.0))*(redLor*redLor)
                           + (1.0-al)*(xVal-(x0+split/2.0))*(blueLor*blueLor));
                }
                else
                {
                    double redGauss = gauss(xVal,x0-split/2.0,width);
                    double blueGauss = gauss(xVal,x0+split/2.0,width);
                    double redExpArg = twoLogTwo*(xVal-x0+split/2.0)*(xVal-x0+split/2.0)/(width*width);
                    double blueExpArg = twoLogTwo*(xVal-x0-split/2.0)*(xVal-x0-split/2.0)/(width*width);

                    dSplit += -2.0*A*twoLogTwo/(width*width)*(al*(xVal-x0+split/2.0)*redGauss
                              -(1.0-al)*(xVal-x0-split/2.0)*blueGauss);
                    dWidth += 4.0*A/width*(al*redGauss*redExpArg + (1.0-al)*blueGauss*blueExpArg);

                    dAi = 2.0*(al*redGauss + (1.0-al)*blueGauss);
                    dAlphai = 2.0*A*(redGauss - blueGauss);
                    dX0i = 4.0*A*twoLogTwo/(width*width)*(al*(xVal-x0+split/2.0)*redGauss + (1.0-al)*(xVal-x0-split/2.0)*blueGauss);
                }

                gradient[offsetIndex+3*j][i] = d2*dAi;
                gradient[offsetIndex+3*j+1][i] = d2*dAlphai;
                gradient[offsetIndex+3*j+2][i] = d2*dX0i;

                gsl_matrix_set(fd->J,i,offsetIndex+3*j,dAi);
                gsl_matrix_set(fd->J,i,offsetIndex+3*j+1,dAlphai);
                gsl_matrix_set(fd->J,i,offsetIndex+3*j+2,dX0i);
            }

            for(int j=0; j<fd->numSingle; j++)
            {
                double A = p.at(offsetIndex+3*fd->numPairs+2*j);
                double x0 = p.at(offsetIndex+3*fd->numPairs+2*j+1);

                double dX0i = 0.0;
                double dAi = 0.0;

                if(fd->lsf == FitResult::Lorentzian)
                {
                    dX0i = 8.0*(xVal-x0)/(width*width)*lor(xVal,x0,width);
                    dAi = lor(xVal,x0,width);
                    dWidth += 8.0*A*(xVal-x0)*(xVal-x0)/(width*width*width)*lor(xVal,x0,width);
                }
                else
                {
                    double G = gauss(xVal,x0,width);
                    double expArg = twoLogTwo*(xVal-x0)*(xVal-x0)/(width*width);

                    dX0i = 2.0*A*G*twoLogTwo*(xVal-x0)/(width*width);
                    dAi = G;
                    dWidth += 2.0*A*G*expArg/width;
                }

                gradient[offsetIndex+3*fd->numPairs+2*j][i] += d2*dAi;
                gradient[offsetIndex+3*fd->numPairs+2*j+1][i] += d2*dX0i;

                gsl_matrix_set(fd->J,i,offsetIndex+3*fd->numPairs+2*j,dAi);
                gsl_matrix_set(fd->J,i,offsetIndex+3*fd->numPairs+2*j+1,dX0i);
            }

            if(fd->type == FitResult::Single)
            {
                gradient[2][i] = d2*dWidth;
                gsl_matrix_set(fd->J,i,2,dWidth);
            }
            else
            {
                gradient[2][i] = d2*dSplit;
                gradient[3][i] = d2*dWidth;

                gsl_matrix_set(fd->J,i,2,dSplit);
                gsl_matrix_set(fd->J,i,3,dWidth);
            }
        }
    }

    //compute Kahan summations of residuals and gradient
    if(!grad.empty())
    {
        for(unsigned int i=0; i<grad.size(); i++)
            grad[i] = Analysis::kahanSum(gradient.at(i));

        fd->lastJacobian = QVector<double>::fromStdVector(grad);
    }

    return Analysis::kahanSum(residualSq);
}

bool AbstractFitter::isFidSaturated(const Scan s)
{
     Fid f = ftw.filterFid(s.fid());
     return isFidSaturated(f);
}

bool AbstractFitter::isFidSaturated(const Fid f)
{
    for(int i=0;i<f.size();i++)
    {
       if(fabs(f.at(i)) > d_fidSaturationLimit)
          return true;
    }
    return false;
}
