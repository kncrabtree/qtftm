#ifndef BATCHATTENUATION_H
#define BATCHATTENUATION_H

#include "batchmanager.h"

class BatchAttenuation : public BatchManager
{
    Q_OBJECT
public:
    explicit BatchAttenuation(double minFreq, double maxFreq, double stepSize, int atten10GHz, Scan s, QString name);
    explicit BatchAttenuation(int num);

    struct TuningValues {
        double frequency;
        int tuningAttn;
        int tuningVoltage;
        int correctedAttn;
        int correctedVoltage;

        TuningValues() : frequency(0.0),  tuningAttn(-1), tuningVoltage(-1), correctedAttn(-1), correctedVoltage(-1) {}
        TuningValues(const TuningValues &other) : frequency(other.frequency), tuningAttn(other.tuningAttn), tuningVoltage(other.tuningVoltage),
            correctedAttn(other.correctedAttn), correctedVoltage(other.correctedVoltage) {}
        TuningValues(const Scan &s) : frequency(s.ftFreq()), tuningAttn(s.attenuation()), tuningVoltage(s.tuningVoltage()),
            correctedAttn(qMax(tuningAttn - (int)round(10.0*log10(1000.0/(double)tuningVoltage)),0)), correctedVoltage(tuningVoltage*pow(10.0,((double)(tuningAttn-correctedAttn)/10.0))) {}

        bool isValid() const { return (frequency > 0.0 && tuningAttn >= 0 && correctedAttn >= 0 && tuningVoltage > 30 && correctedVoltage > 0); }
        QString toString() const { return QString("%1\t%2\t%3\t%4\t%5").arg(frequency,0,'f',3).arg(tuningAttn).arg(tuningVoltage).arg(correctedAttn).arg(correctedVoltage); }
        bool operator <(const TuningValues &other) { return frequency < other.frequency; }
        bool operator >(const TuningValues &other) { return frequency > other.frequency; }
        bool operator ==(const TuningValues &other) { return fabs(frequency-other.frequency) < 0.001; }
        bool operator !=(const TuningValues &other) { return fabs(frequency-other.frequency) >= 0.001; }
        TuningValues& operator =(const TuningValues& other) { frequency = other.frequency; tuningAttn = other.tuningAttn; tuningVoltage = other.tuningVoltage;
            correctedAttn = other.correctedAttn; correctedVoltage = other.correctedVoltage; return *this; }
    };
    
signals:
    void elementComplete();
    
public slots:
    Scan prepareNextScan();
    bool isBatchComplete();
    void processScan(Scan s);
    void writeReport();

    void abort() { d_aborted = true; }

private:
    bool d_scanUpComplete;
    bool d_scanDownComplete;
    int d_scanUpIndex;
    int d_scanDownIndex;
    Scan d_template;
    QString d_atnFilename;

    double d_minFreq;
    double d_maxFreq;
    double d_stepSize;
    int d_atten10GHz;

    bool d_retrying;
    bool d_aborted;
    int d_nextAttn;

    QList<TuningValues> d_tuningValues;
    QList<QVector<QPointF> > d_plotData;
    
};

#endif // BATCHATTENUATION_H
