#ifndef ANTONUCCIMOTORDRIVER_H
#define ANTONUCCIMOTORDRIVER_H

#include "motordriver.h"

class AntonucciMotorDriver : public MotorDriver
{
    Q_OBJECT
public:
    explicit AntonucciMotorDriver(QObject *parent = nullptr);

    // HardwareObject interface
public slots:
    void initialize();
    bool testConnection();

    // MotorDriver interface
public slots:
    void tune(double freq, int currentAttn, int mode);
    QStringList roughTune(int posGuess, bool calibrating = false, double freq = -1); // let roughTune know freq so it can change if F in top region Mar 20 PRAA
    QByteArray intermediateTune();
    bool fineTune(int targetVoltage, int width, double freq, int currentMode, int currentAttn);
    void calibrate();

    // MotorDriver interface
protected:
    bool moveToPosition(int pos);
    bool stepMotor(int motorSteps);
    int readAnalog();
    int readPos();

    int getAnalogReading();

};

#endif // ANTONUCCIMOTORDRIVER_H
