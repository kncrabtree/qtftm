#ifndef FLOWCONTROLLER_H
#define FLOWCONTROLLER_H

#include "rs232instrument.h"
#include <QTimer>

class FlowController : public Rs232Instrument
{
	Q_OBJECT
public:
    explicit FlowController(QObject *parent = nullptr);

    struct FlowChannels {
        double pressure;
        double flow1;
        double flow2;
        double flow3;
        double flow4;
        int pRange;
        int fRange1;
        int fRange2;
        int fRange3;
        int fRange4;
        double gcf1;
        double gcf2;
        double gcf3;
        double gcf4;

        FlowChannels() : pressure(0.0), flow1(0.0), flow2(0.0), flow3(0.0), flow4(0.0),
            pRange(0), fRange1(0), fRange2(0), fRange3(0), fRange4(0),
            gcf1(1.0), gcf2(1.0), gcf3(1.0), gcf4(1.0) {}
        FlowChannels(const FlowChannels &other) : pressure(other.pressure), flow1(other.flow1),
            flow2(other.flow2), flow3(other.flow3), flow4(other.flow4),
            pRange(other.pRange), fRange1(other.fRange1), fRange2(other.fRange2), fRange3(other.fRange3),
            fRange4(other.fRange4), gcf1(other.gcf1), gcf2(other.gcf2), gcf3(other.gcf3), gcf4(other.gcf4) {}

    };

    enum FlowIndex {
        Pressure,
        Flow1,
        Flow2,
        Flow3,
        Flow4
    };
	
signals:
    void flowUpdated(FlowController::FlowIndex, double);
    void setPointUpdated(FlowController::FlowIndex, double);
    void pressureControlMode(bool);
	
public slots:
    void initialize();
	bool testConnection();
    void sleep(bool b);
    void updateFlows();
    void setSetPoint(FlowController::FlowIndex i, double val);
    void setPressureControl(bool on);
    void readPressureControlStatus();

    FlowController::FlowChannels readFlows() const { return FlowChannels(d_flows); }

private:
    bool d_connected;
    QList<double> d_gasRangeList;
    QList<double> d_pressureRangeList;
    FlowChannels d_flows;

    FlowIndex d_index;
    QTimer *flowTimer;

    void pressureQuery();
    void flowQuery(int ch);
    void queryPressureSetPoint();
    void queryFlowSetPoint(int ch);
    void setPressureSetPoint(double val);
    void setFlowSetPoint(int ch, double val);
	
};

#endif // FLOWCONTROLLER_H
