#include "abstractfitter.h"

#include <eigen3/Eigen/SVD>

AbstractFitter::AbstractFitter(const FitResult::FitterType t, QObject *parent) :
    QObject(parent), d_type(t), d_window(11), d_polyOrder(6)
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

QList<QPair<QPointF,double>> AbstractFitter::findPeaks(QVector<QPointF> ft, double noisey0, double noisem, double minSNR)
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
        if(thisSNR >= minSNR)
        {
            //intensity is high enough; ID a peak by a minimum in 2nd derivative
            if(((smth.at(i-2) > smth.at(i-1)) && (smth.at(i-1) > smth.at(i)) && (smth.at(i) < smth.at(i+1))) ||
                    ((smth.at(i-1) > smth.at(i)) && (smth.at(i) < smth.at(i+1)) && (smth.at(i+1) < smth.at(i+2))) )
                out.append(qMakePair(ft.at(i),thisSNR));
        }
    }

    return out;
}

bool AbstractFitter::isFidSaturated(const Scan s)
{
     return Analysis::isFidSaturated(ftw.filterFid(s.fid()));
}
