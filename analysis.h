#ifndef ANALYSIS_H
#define ANALYSIS_H
#include <QList>
#include <QVector>
#include <QPair>
#include <QPointF>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_const_cgs.h>
#include "fid.h"
#include <gsl/gsl_multifit_nlin.h>
#include "fitresult.h"
#include "dopplerpair.h"

namespace Analysis {

/*******************************
 * FITTING/ANALYSIS PARAMETERS *
 *******************************/

const int baselineMedianBinSize = 10;
const double twoLogTwo = 2.0*log(2.0);
const double rootTwoLogTwo = sqrt(twoLogTwo);

/******************************
 * ENUM/STRUCTURE DEFINITIONS *
 ******************************/

enum Coordinate {
    Xcoord,
    Ycoord
};

/***********************************
 *    GENERAL-PURPOSE ROUTINES     *
 ***********************************/
FitResult::BufferGas bgFromString(const QString bgName);
QVector<double> extractCoordinateVector(const QVector<QPointF> data, Coordinate c);
double median(const QVector<double> data);
double median(const QVector<QPointF> data, Coordinate c);
QPointF median(const QVector<QPointF> data);
double mean(const QVector<double> data);
double mean(const QVector<QPointF> data, Coordinate c);
QPointF mean(const QVector<QPointF> data);
double variance(const QVector<double> data);
double variance(const QVector<QPointF> data, Coordinate c);
QPointF variance(const QVector<QPointF> data);
double stDev(const QVector<double> data);
double stDev(const QVector<QPointF> data, Coordinate c);
QPointF stDev(const QVector<QPointF> data);
Fid removeDC(const Fid f);
unsigned int power2Nplus1(unsigned int n);
QList<double> estimateBaseline(const QVector<QPointF> ftData);
QVector<QPointF> removeBaseline(const QVector<QPointF> data, double y0, double slope, double probeFreq = 0.0);
bool dpAmplitudeLess(const FitResult::DopplerPairParameters &left, const FitResult::DopplerPairParameters &right);
double estimateLinewidth(const FitResult::BufferGas &bg, double probeFreq, double stagT = 293.15);
void estimateDopplerPairAmplitude(const QVector<QPointF> ft, DopplerPair *dp, QPair<double,double> baseline);
double kahanSum(const QVector<double> dat);

//Lineshape functions
double lor(double x, double center, double fwhm);
double gauss(double x, double center, double fwhm);

/*******************************
 *   OSCILLOSCOPE PARSING      *
 ******************************/
Fid parseWaveform(const QByteArray d, double probeFreq);

}

#endif // ANALYSIS_H
