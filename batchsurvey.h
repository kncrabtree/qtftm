#ifndef BATCHSURVEY_H
#define BATCHSURVEY_H

#include "batchmanager.h"

/*!
 * \brief An implementation of BatchManager that scans the FT frequency across a continuous spectral region.
 *
 * The BatchSurvey class contains a reference scan that contains all settings for the FT except the FT frequency (in the future, perhaps also the attenuation?).
 * The FT frequency is calculated from the start and step frequencies, and the survey is complete when the stop frequency is scanned.
 * In addition, there is an option to do periodic calibration scans.
 * If so, a reference Scan is used to store all the settings for that scan, and the scansPerCal variable sets how many scans are executed between calibrations.
 * The survey will always begin and end with a calibration scan if they are enabled.
 * Survey reports are stored in savePath/surveys/x/y/z.txt, where z is the survey number, y are the thousands digits, and x are the millions digits (though it's unlikely that will ever go above 0!)
 * Reports contain lists of survey and calibration scan numbers, as well as XY lists of frequency and FT intensity for the survey, and frequency and peak calibration line intensity for the calibrations.
 */
class BatchSurvey : public BatchManager
{
	Q_OBJECT
public:
    explicit BatchSurvey(Scan first, double step, double end, bool hascal = false, Scan cal = Scan(), int scansPerCal = 0, AbstractFitter *af = new NoFitter());
    explicit BatchSurvey(int num);
	
signals:
	
public slots:

private:
	Scan d_surveyTemplate;
	double d_step;
	double d_chunkStart;
	double d_chunkEnd;
	double d_offset;
	bool d_hasCalibration;
	Scan d_calTemplate;
	int d_scansPerCal;
    int d_totalSurveyScans;
    int d_totalCalScans;
    int d_currentSurveyIndex;
    bool d_processScanIsCal;

    QVector<QPointF> d_calData;
    QVector<QPointF> d_surveyData;
    QList<int> d_surveyScanNumbers;
    QList<int> d_calScanNumbers;


protected:
	Scan prepareNextScan();
	bool isBatchComplete();
    void advanceBatch(const Scan s);
	void processScan(Scan s);
	void writeReport();

private:
	QString makeHeader(int num);
	
};

#endif // BATCHSURVEY_H
