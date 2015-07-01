#include "flowcontroller.h"
#include <math.h>

FlowController::FlowController(QObject *parent) :
    Rs232Instrument(QString("flow"), QString("Flow Control Unit"), parent), d_connected(false), d_index(Pressure)
{
    double b = 28316.847; //scfm --> sccm conversion
    double c = b/60.0; // scfh --> sccm conversion

    //Gas ranges. see manual for details. All are in SCCM
    d_gasRangeList << 1.0 << 2.0 << 5.0 << 10.0 << 20.0 << 50.0 << 100.0 << 200.0 << 500.0
                   << 1e3 << 2e3 << 5e3 << 1e4 << 2e4 << 5e4 << 1e5 << 2e5 << 4e5 << 5e5 << 1e6
                   << c << 2.0*c << 5.0*c << 1e1*c << 2e1*c << 5e1*c << 1e2*c << 2e2*c << 5e2*c
                   << b << 2.0*b << 5.0*b << 1e1*b << 2e1*b << 5e1*b << 1e2*b << 2e2*b << 5e2*b
                   << 30e3 << 300e3;

    double d = 0.750061683; //bar --> kTorr conversion
    double f = d*1e-5; // Pa --> kTorr conversion

    //pressure ranges. All are in kTorr. Some are redundant (same value, different units in manual. e.g. 3 = 1000 mTorr, 4 = 1 Torr)
    d_pressureRangeList << 1e-6 << 10e-6 << 100e-6 << 1000e-6 << 1e-3 << 10e-3 << 100e-3 << 1000e-3
                        << 1.0 << 1e1 << 1e2 << 1e-6*d << 10e-6*d << 100e-6*d << 1000e-6*d
                        << 1e-3*d << 10e-3*d << 100e-3*d << 1000e-3*d << d << 1e1*d << 1e2*d
                        << f << 1e1*f << 1e2*f << 1e3*f << 1e4*f << 1e5*f << 1e6*f;


}


void FlowController::initialize()
{
    setReadOptions(1000,true,QByteArray("\r\n"));
    flowTimer = new QTimer(this);
    connect(flowTimer,&QTimer::timeout,this,&FlowController::updateFlows);

    Rs232Instrument::initialize();
    testConnection();

}


bool FlowController::testConnection()
{
    d_connected = false;
    flowTimer->stop();
    if(!Rs232Instrument::testConnection())
    {
        emit connected(false,QString("RS232 error."));
        return false;
    }

    blockSignals(true);
    QByteArray resp = queryCmd(QString("ID;\r\n"));
    blockSignals(false);

    if(resp.isEmpty())
    {
        //try again
        resp = queryCmd(QString("ID;\r\n"));
        if(resp.isEmpty())
        {
            emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
            return false;
        }
    }

    if(!resp.startsWith("MGC 647"))
    {
        //try again... stupid firmware bug sometimes causes responses to be repeated twice
        bool done = false;
        while(!done)
        {
            resp.clear();
            resp = queryCmd(QString("ID;\r\n"));
            if(resp.isEmpty())
            {
                emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
                return false;
            }
            if(resp.startsWith("MGC 647"))
                done = true;
            else
            {
                emit connected(false,QString("%1 gave a invalid response to ID query (response = %2)").arg(d_prettyName).arg(QString(resp)));
                return false;
            }
        }
    }
    emit logMessage(QString("%1 ID response: %2").arg(d_prettyName).arg(QString(resp)));
    emit logMessage(QString("Reading all %1 settings. This will take a few seconds...").arg(d_prettyName));

    d_connected = true;


    readPressureControlStatus();
    pressureQuery();
    flowQuery(1);
    flowQuery(2);
    flowQuery(3);
    flowQuery(4);
    queryPressureSetPoint();
    queryFlowSetPoint(1);
    queryFlowSetPoint(2);
    queryFlowSetPoint(3);
    queryFlowSetPoint(4);

    flowTimer->start(333);

    emit connected();
    return true;
}

void FlowController::sleep(bool b)
{
    if(d_connected)
    {
        if(b)
        {
            setPressureControl(false);
//            writeCmd(QString("PM0;\r\n"));
            writeCmd(QString("OF0;\r\n"));
        }
        else
        {
//            writeCmd(QString("PM1;\r\n"));
            writeCmd(QString("ON0;\r\n"));
        }
    }

    HardwareObject::sleep(b);
}

void FlowController::updateFlows()
{
    if(!d_connected)
        return;

    switch(d_index)
    {
    case Pressure:
        pressureQuery();
        d_index = Flow1;
        emit flowUpdated(Pressure,d_flows.pressure);
        break;
    case Flow1:
        flowQuery(1);
        d_index = Flow2;
        emit flowUpdated(Flow1,d_flows.flow1);
        break;
    case Flow2:
        flowQuery(2);
        d_index = Flow3;
        emit flowUpdated(Flow2,d_flows.flow2);
        break;
    case Flow3:
        flowQuery(3);
        d_index = Flow4;
        emit flowUpdated(Flow3,d_flows.flow3);
        break;
    case Flow4:
        flowQuery(4);
        d_index = Pressure;
        emit flowUpdated(Flow4,d_flows.flow4);
        break;
    }

}

void FlowController::setSetPoint(FlowController::FlowIndex i, double val)
{
    if(!d_connected)
        return;

    switch(i)
    {
    case Pressure:
        setPressureSetPoint(val);
        break;
    case Flow1:
        setFlowSetPoint(1,val);
        break;
    case Flow2:
        setFlowSetPoint(2,val);
        break;
    case Flow3:
        setFlowSetPoint(3,val);
        break;
    case Flow4:
        setFlowSetPoint(4,val);
        break;
    }
}

void FlowController::setPressureControl(bool on)
{
    if(!d_connected)
        return;

    if(on)
        writeCmd(QString("PM1;\r\n"));
    else
        writeCmd(QString("PM0;\r\n"));

    //now, query state
    readPressureControlStatus();
}

void FlowController::readPressureControlStatus()
{
    QByteArray resp = queryCmd(QString("PMR;\r\n")).trimmed();
    if(resp.length() != 1)
    {
        int tries = 1;
        bool done = false;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 1)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to pressure control mode query.").arg(d_prettyName),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    bool ok = false;
    int i = resp.toInt(&ok);
    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not parse pressure control mode response from %1. Response: %2").arg(d_prettyName).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    if(i)
        emit pressureControlMode(true);
    else
        emit pressureControlMode(false);
}

void FlowController::pressureQuery()
{
    //read pressure range
    QByteArray resp = queryCmd(QString("PUR;\r\n")).trimmed();
    if(resp.length() != 2) //workaround for firmware bug
    {
        int tries = 1;
        bool done = false;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 2)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to pressure range query.").arg(d_prettyName),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    bool ok = false;
    int i = resp.trimmed().toInt(&ok);
//    emit logMessage(QString("PUR response: %1").arg(QString(resp)));
    if(!ok || i >= d_pressureRangeList.size() || i<0)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read pressure gauge range from %1. Response: %2").arg(d_prettyName).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    //now read pressure
    resp.clear();
    resp = queryCmd(QString("PR;\r\n")).trimmed();
    if(resp.length() != 5) //workaround for firmware bug
    {
        bool done = false;
        int tries = 1;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 5)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to pressure query.").arg(d_prettyName),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    ok = false;
    double d = resp.toDouble(&ok);
//    emit logMessage(QString("PR response: %1").arg(QString(resp)));
    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read pressure from %1. Response: %2").arg(d_prettyName).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    //pressure is in units of 0.1% of full scale
    d_flows.pressure = d*d_pressureRangeList.at(i)*1e-3;
    d_flows.pRange = i;
}

void FlowController::flowQuery(int ch)
{
    //read flow range
    QByteArray lastResp;
    QByteArray resp = queryCmd(QString("RA%1R;\r\n").arg(ch)).trimmed();
    if(resp.length() != 2) //workaround for firmware bug
    {
        int tries = 1;
        bool done = false;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 2)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to flow range query for channel %2.").arg(d_prettyName).arg(ch),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    bool ok = false;
    int i = resp.toInt(&ok);
//    emit logMessage(QString("RA%1R response: %2").arg(ch).arg(QString(resp)));
    if(!ok || i >= d_gasRangeList.size() || i<0)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read flow range from %1 for channel %2. Response: %3").arg(d_prettyName).arg(ch).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }


    //now read gas correction factor
    resp.clear();
    resp = queryCmd(QString("GC%1R;\r\n").arg(ch)).trimmed();
    if(resp.length() != 5) //workaround for firmware bug
    {
        int tries = 1;
        bool done = false;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 5)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to correction factor query for channel %2.").arg(d_prettyName).arg(ch),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    ok = false;
    double gcf = resp.toDouble(&ok)/100.0;
//    emit logMessage(QString("GC%1R response: %2").arg(ch).arg(QString(resp)));
    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read correction factor from %1 for channel %2. Response: %3").arg(d_prettyName).arg(ch).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    lastResp = resp;
    //now read flow
    resp.clear();
    resp = queryCmd(QString("FL%1R;\r\n").arg(ch)).trimmed();
//    if(resp == lastResp) //workaround for firmware bug
//    {
//        //try one more time
//        resp.clear();
//        resp = queryCmd(QString("\r\n")).trimmed();
//    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to flow query for channel %2.").arg(d_prettyName).arg(ch),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    ok = false;
    double f = resp.toDouble(&ok);
//    emit logMessage(QString("FL%1R response: %2").arg(ch).arg(QString(resp)));
    if(!ok && resp != "-----")
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read flow from %1 for channel %2. Response: %3").arg(d_prettyName).arg(ch).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    //convert readings into flow in sccm. f is in 0.1% of full scale, and gcf is fractional
    double flow = -999.9;
    if(ok)
        flow = (f*1e-3*d_gasRangeList.at(i))*gcf;
    else //flow is off scale; return max value
        flow = d_gasRangeList.at(i)*gcf;


    switch(ch)
    {
    case 1:
        d_flows.flow1 = flow;
        d_flows.fRange1 = i;
        d_flows.gcf1 = gcf;
        break;
    case 2:
        d_flows.flow2 = flow;
        d_flows.fRange2 = i;
        d_flows.gcf2 = gcf;
        break;
    case 3:
        d_flows.flow3 = flow;
        d_flows.fRange3 = i;
        d_flows.gcf3 = gcf;
        break;
    case 4:
        d_flows.flow4 = flow;
        d_flows.fRange4 = i;
        d_flows.gcf4 = gcf;
        break;
    }

}

void FlowController::queryPressureSetPoint()
{
     //although we already know the pressure range, we need to query it again because of the stupid firmware bug
    QByteArray resp = queryCmd(QString("PUR;\r\n")).trimmed();
    if(resp.length() != 2) //workaround for firmware bug
    {
        int tries = 1;
        bool done = false;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 2)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to pressure range query.").arg(d_prettyName),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    //we don't care about the pressure range....

    //now read pressure serpoint
    resp.clear();
    resp = queryCmd(QString("PSR;\r\n")).trimmed();
    if(resp.length() != 5) //workaround for firmware bug
    {
        bool done = false;
        int tries = 1;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 5)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to pressure query.").arg(d_prettyName),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    bool ok = false;
    double d = resp.toDouble(&ok);
//    emit logMessage(QString("PR response: %1").arg(QString(resp)));
    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read pressure from %1. Response: %2").arg(d_prettyName).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    //pressure is in units of 0.1% of full scale
    emit setPointUpdated(Pressure,d*d_pressureRangeList.at(d_flows.pRange)*1e-3);
}

void FlowController::queryFlowSetPoint(int ch)
{
    //although we already know the flow range, we need to query it again because of the stupid firmware bug
    QByteArray resp = queryCmd(QString("RA%1R;\r\n").arg(ch)).trimmed();
    if(resp.length() != 2) //workaround for firmware bug
    {
        int tries = 1;
        bool done = false;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 2)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to flow range query for channel %2.").arg(d_prettyName).arg(ch),QtFTM::LogError);
        //d_connected = false;
        return;
    }
    //we don't actually care about the range right now...

    //now read the setpoint
    resp.clear();
    resp = queryCmd(QString("FS%1R;\r\n").arg(ch)).trimmed();
    if(resp.length() != 5) //workaround for firmware bug
    {
        int tries = 1;
        bool done = false;
        while(!done)
        {
            tries++;
            resp.clear();
            resp = queryCmd(QString("\r\n")).trimmed();
            if(resp.isEmpty() || resp.length() == 5)
                done = true;
        }
//        emit logMessage(QString("Took %1 tries to get valid response").arg(tries));
    }
    if(resp.isEmpty())
    {
        emit hardwareFailure();
        emit logMessage(QString("%1 did not respond to setpoint query for channel %2.").arg(d_prettyName).arg(ch),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    bool ok = false;
    double sp = resp.toDouble(&ok)/1000.0;
//    emit logMessage(QString("GC%1R response: %2").arg(ch).arg(QString(resp)));
    if(!ok)
    {
        emit hardwareFailure();
        emit logMessage(QString("Could not read setpoint from %1 for channel %2. Response: %3").arg(d_prettyName).arg(ch).arg(QString(resp)),QtFTM::LogError);
        //d_connected = false;
        return;
    }

    if(resp.toInt() == 0)
        writeCmd(QString("OF%1;\r\n").arg(ch));
    else
        writeCmd(QString("ON%1;\r\n").arg(ch));

    switch(ch)
    {
    case 1:
        emit setPointUpdated(Flow1,sp*d_gasRangeList.at(d_flows.fRange1)*d_flows.gcf1);
        break;
    case 2:
        emit setPointUpdated(Flow2,sp*d_gasRangeList.at(d_flows.fRange2)*d_flows.gcf2);
        break;
    case 3:
        emit setPointUpdated(Flow3,sp*d_gasRangeList.at(d_flows.fRange3)*d_flows.gcf3);
        break;
    case 4:
        emit setPointUpdated(Flow4,sp*d_gasRangeList.at(d_flows.fRange4)*d_flows.gcf4);
        break;
    }
}

void FlowController::setPressureSetPoint(double val)
{
    //make sure we have updated range settings
    pressureQuery();
    //use full scale range to convert val to setpoint in units of 0.1% full scale
    int sp = (int)round(val/d_pressureRangeList.at(d_flows.pRange)*1000.0);
    if(sp >=0 && sp <=1100)
        writeCmd(QString("PS%1;\r\n").arg(sp,4,10,QLatin1Char('0')));

    queryPressureSetPoint();

}

void FlowController::setFlowSetPoint(int ch, double val)
{
    //make sure range and gcf are updated
    flowQuery(ch);
    //use full scale range and gcf to convert val to setpoint in units of 0.1% full scale
    int sp = -1;
    switch(ch)
    {
    case 1:
        sp = (int)round(val/(d_gasRangeList.at(d_flows.fRange1)*d_flows.gcf1)*1000.0);
        break;
    case 2:
        sp = (int)round(val/(d_gasRangeList.at(d_flows.fRange2)*d_flows.gcf2)*1000.0);
        break;
    case 3:
        sp = (int)round(val/(d_gasRangeList.at(d_flows.fRange3)*d_flows.gcf3)*1000.0);
        break;
    case 4:
        sp = (int)round(val/(d_gasRangeList.at(d_flows.fRange4)*d_flows.gcf4)*1000.0);
        break;
    }

    if(sp >=0 && sp <=1100)
        writeCmd(QString("FS%1%2;\r\n").arg(ch).arg(sp,4,10,QLatin1Char('0')));

    queryFlowSetPoint(ch);
}
