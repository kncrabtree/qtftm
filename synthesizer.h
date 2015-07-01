#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#include "gpibinstrument.h"

class Synthesizer : public GpibInstrument
{
    Q_OBJECT
public:
    explicit Synthesizer(QString key, QString name, QTcpSocket *s, QObject *parent = nullptr);
    
signals:
    void rangeChanged();
    void triggerBandChange(int);
    
public slots:
    void initialize();
    void updateBandFromUi();

protected:
    virtual double realToRaw(double f);
    virtual double rawToReal(double f);
    virtual double readFreq() =0;
    bool calculateBand(double f);
    void switchBand(int band);

    double d_mult;
    double d_offset;
    double d_hardwareMinFreq;
    double d_hardwareMaxFreq;
    
};

#endif // SYNTHESIZER_H
