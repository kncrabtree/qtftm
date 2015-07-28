#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#include "hardwareobject.h"

class Synthesizer : public HardwareObject
{
    Q_OBJECT
public:
    explicit Synthesizer(QObject *parent = nullptr);
    virtual ~Synthesizer();
    
signals:
    void rangeChanged();
    void triggerBandChange(int);
    void frequencyUpdate(double);
    void powerUpdate(double);
    
public slots:
    virtual void initialize();
    void updateBandFromUi();

    double readFreq();
    double setFreq(double d);
    double readPower();
    double setPower(double p);


protected:
    virtual double realToRaw(double f);
    virtual double rawToReal(double f);   
    virtual double setSynthFreq(double d) =0;
    virtual double readSynthFreq() =0;
    virtual double setSynthPower(double d) =0;
    virtual double readSynthPower() =0;

    bool calculateBand(double f);
    void switchBand(int band);
    double setFreqForce(double f); //doesn't call calculateBand()

    double d_mult;
    double d_offset;
    double d_hardwareMinFreq;
    double d_hardwareMaxFreq;
    double d_hardwareMinPower;
    double d_hardwareMaxPower;
    
};

#endif // SYNTHESIZER_H
