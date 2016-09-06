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

const int smoothFactor = 10;
const double peakFindingSNR = 3.0;
const double fidSaturationLimit = 0.6; // |voltage| at which scope saturates
const int baselineMedianBinSize = 10;
const double splittingTolerance = 0.35;
const double alphaTolerance = 0.10;
const double edgeSkepticalWRTSplitting = 0.75;
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
QVector<QPointF> boxcarSmooth(const QVector<QPointF> data, int numPoints);
QList<QPair<QPointF, double> > findPeaks(const QVector<QPointF> data, double noiseY0, double noiseSlope, double snr);
bool isFidSaturated(const Fid f);
Fid removeDC(const Fid f);
unsigned int power2Nplus1(unsigned int n);
QList<double> estimateBaseline(const QVector<QPointF> ftData);
QVector<QPointF> removeBaseline(const QVector<QPointF> data, double y0, double slope, double probeFreq = 0.0);
double estimateSplitting(const FitResult::BufferGas &bg, double stagT, double frequency);
QList<FitResult::DopplerPairParameters> estimateDopplerCenters(QList<QPair<QPointF, double> > peakList, double splitting, double ftSpacing);
bool dpAmplitudeLess(const FitResult::DopplerPairParameters &left, const FitResult::DopplerPairParameters &right);
double estimateDopplerLinewidth(const FitResult::BufferGas &bg, double probeFreq, double stagT = 293.15);
void estimateDopplerPairAmplitude(const QVector<QPointF> ft, DopplerPair *dp, QPair<double,double> baseline);
double kahanSum(const QVector<double> dat);


/***********************************
 *    FITTING FUNCTIONS FOR GSL    *
 ***********************************/

struct MixedDopplerData {
	QVector<QPointF> data;
	int numPairs;
	int numSinglePeaks;
};

double lor(double x, double center, double fwhm);
double gauss(double x, double center, double fwhm);
int lorDopplerPair_f(const gsl_vector * x, void *data, gsl_vector * f);
int lorDopplerPair_df(const gsl_vector * x, void *data, gsl_matrix * J);
int lorDopplerPair_fdf(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);
int gaussDopplerPair_f(const gsl_vector * x, void *data, gsl_vector * f);
int gaussDopplerPair_df(const gsl_vector * x, void *data, gsl_matrix * J);
int gaussDopplerPair_fdf(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);
int lorDopplerMixed_f(const gsl_vector * x, void *data, gsl_vector * f);
int lorDopplerMixed_df(const gsl_vector * x, void *data, gsl_matrix * J);
int lorDopplerMixed_fdf(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);
int gaussDopplerMixed_f(const gsl_vector * x, void *data, gsl_vector * f);
int gaussDopplerMixed_df(const gsl_vector * x, void *data, gsl_matrix * J);
int gaussDopplerMixed_fdf(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);
int lorSingle_f(const gsl_vector * x, void *data, gsl_vector * f);
int lorSingle_df(const gsl_vector * x, void *data, gsl_matrix * J);
int lorSingle_fdf(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);
int gaussSingle_f(const gsl_vector * x, void *data, gsl_vector * f);
int gaussSingle_df(const gsl_vector * x, void *data, gsl_matrix * J);
int gaussSingle_fdf(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);

FitResult dopplerPairFit(const gsl_multifit_fdfsolver_type *solverType, FitResult::LineShape lsf, const QVector<QPointF> data, const double probeFreq, const double y0, const double slope, const double split, const double width, const QList<FitResult::DopplerPairParameters> dpParams, const int maxIterations = 50);
FitResult dopplerMixedFit(const gsl_multifit_fdfsolver_type *solverType, FitResult::LineShape lsf, const QVector<QPointF> data, const double probeFreq, const double y0, const double slope, const double split, const double width, const QList<FitResult::DopplerPairParameters> dpParams, const QList<QPointF> singleParams, const int maxIterations = 50);
FitResult singleFit(const gsl_multifit_fdfsolver_type *solverType, FitResult::LineShape lsf, const QVector<QPointF> data, const double probeFreq, const double y0, const double slope, const double width, const QList<QPointF> singleParams, const int maxIterations = 50);
FitResult fitLine(const FitResult &in, QVector<QPointF> data, double probeFreq);

/*******************************
 *   OSCILLOSCOPE PARSING      *
 ******************************/
Fid parseWaveform(const QByteArray d, double probeFreq);

}

#endif // ANALYSIS_H
