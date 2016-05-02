#ifndef GLASSMANFJ02R60_H
#define GLASSMANFJ02R60_H

#include "hvpowersupply.h"

class GlassmanFJ02R60 : public HvPowerSupply
{
public:
    GlassmanFJ02R60();

    // HardwareObject interface
public slots:
    bool testConnection();
    void initialize();

    // HvPowerSupply interface
protected:
    bool hwSetVoltage(int v);
    int hwReadVoltage();

private:
    int d_fullScaleVoltage;
    int d_fullScaleCurrent;
    int d_currentVoltage;

    QByteArray calculateChecksum(QByteArray cmd);
    QString formatMessage(QString cmd);
    int sendSetCommand(QString cmd);
    QByteArray sendQueryCmd();
    void sendErrorMessage(int errCode);
    bool checkFault();
};

#endif // GLASSMANFJ02R60_H
