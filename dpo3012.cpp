#include "dpo3012.h"

#include "tcpinstrument.h"

#include <QTimer>

DPO3012::DPO3012(QObject *parent) :
    Oscilloscope(parent)
{
    d_subKey = QString("dpo3012");
    d_prettyName = QString("DPO3012 Oscilloscope");

    p_comm = new TcpInstrument(d_key,d_subKey,this);
    connect(p_comm,&CommunicationProtocol::logMessage,this,&DPO3012::logMessage);
    connect(p_comm,&CommunicationProtocol::hardwareFailure,this,&DPO3012::hardwareFailure);
}

DPO3012::~DPO3012()
{
    if(p_socket->state() == QAbstractSocket::ConnectedState)
        p_comm->writeCmd(QString("UNLOCK ALL\n"));
}

void DPO3012::initialize()
{
    p_comm->setReadOptions(1000,true,QByteArray("\n\r"));
    p_comm->initialize();
    p_socket = dynamic_cast<QTcpSocket*>(p_comm->device());
    testConnection();
}

bool DPO3012::testConnection()
{
    d_waitingForWakeUp = false;
    setActive(false);
    disconnect(p_socket,&QIODevice::readyRead,this,&DPO3012::replyReceived);

    if(!p_comm->testConnection())
    {
        emit connected(false);
        return false;
    }

    QByteArray resp = p_comm->queryCmd(QString("*IDN?\n"));

    if(resp.isEmpty())
    {
        emit connected(false,QString("Did not respond to ID query."));
        return false;
    }

    if(!resp.startsWith(QByteArray("TEKTRONIX,DSA71604C")))
    {
        emit connected(false,QString("ID response invalid. Response: %1 (Hex: %2)").arg(QString(resp)).arg(QString(resp.toHex())));
        return false;
    }

    emit logMessage(QString("ID response: %1").arg(QString(resp)));

    configure();

    if(p_socket->bytesAvailable())
        p_socket->readAll();

    //initialize acquisition variables
    d_headerHashFound = false;
    d_binaryDataLength = -1;

    //set resolution
    setResolution();

    setActive(true);

    connect(p_socket,&QIODevice::readyRead,this,&DPO3012::replyReceived,Qt::UniqueConnection);

    emit connected();
    return true;
}

void DPO3012::configure()
{
    //lock front panel
    p_comm->writeCmd(QString("LOCK ALL;DCL\n"));

    //make appropriate settings

    //first, stop acquisition
    p_comm->writeCmd(QString("ACQUIRE:STATE OFF\n"));

    //use channel 1 at full bandwidth, AC coupled, 125 mV/div (1 V full scale), units in V, offset 0
    p_comm->writeCmd(QString("CH1:BANDWIDTH FULL;COUPLING AC;OFFSET 0;SCALE 0.125;YUNITS 'V'\n"));

    //triggering settings
    p_comm->writeCmd(QString("TRIGGER:A:TYPE EDGE;MODE NORMAL;LEVEL TTL;HOLDOFF:TIME 50e-6\n"));
    p_comm->writeCmd(QString("TRIGGER:A:EDGE:COUPLING DC;SLOPE FALL;SOURCE AUX\n"));

    //horizontal settings (except those pertaining to resolution)
    p_comm->writeCmd(QString("HORIZONTAL:DELAY:MODE OFF;:HORIZONTAL:POSITION 0\n"));

    //data transfer settings
    p_comm->writeCmd(QString("DATA:SOURCE CH1;START 1;WIDTH 2;ENCDG BIN\n"));

    //waveform transfer settings
    p_comm->writeCmd(QString("WFMOUTPRE:ENCDG BIN;BN_FMT RI;BYT_OR MSB;BYT_NR 2\n"));

    //acquitision settings
    p_comm->writeCmd(QString("ACQUIRE:MODE HIRES;STOPAFTER RUNSTOP\n"));

    //begin acquisition
    p_comm->writeCmd(QString("ACQUIRE:STATE ON\n"));

    if(p_socket->bytesAvailable())
        p_socket->readAll();
}

void DPO3012::replyReceived()
{
    //this function is called when bytes are available at the TCP socket.
    //The scope manual describes the response format in more detail, but briefly, it looks like #xyyyy<data>\n\r
    //where x is an integer 1-9 that tells how many y digits follow, and yyyy is the number of bytes of binary data that follow before the \n\r

    //first step is to find the header hash (#)
    if(!d_headerHashFound)
    {
//        emit logMessage(QString("Looking for hash... (%1 ms)").arg(timeTest.elapsed()));

        //while there are bytes that can be read, read them and look for a hash
        while(p_socket->bytesAvailable()>0) // we can ignore anything that is before the hash
        {
            char c;
            p_socket->getChar(&c);
            if(c == '#')
            {
                //put hash into response
                d_response.append(c);
//                emit logMessage(QString("Found hash. Time elapsed: %1").arg(timeTest.elapsed()));
                d_headerHashFound = true;
                break;
            }
        }
    }

    if(d_headerHashFound && d_binaryDataLength < 0) // we've found the header, now we need to read in the size of the binary data
    {
        //after the hash, the next character is a digit telling the how many digits are in the number of bytes of binary data to follow.
        //We need to make sure there are still at least that many bytes available so we can parse the number of binary data bytes
        char c;
        p_socket->getChar(&c);
        bool ok = true;
        int numHeaderBytes = QByteArray(&c,1).toInt(&ok);
        if(!ok)
        {
            //something went wrong; perhaps we're in the midst of some binary data or something. Ignore this and look for a new hash
            d_headerHashFound = false;
            d_response.clear();
            return;
        }

        if(numHeaderBytes > p_socket->bytesAvailable()) // we don't have enough bytes! put that character back, and we'll try again when more data comes in
        {
            p_socket->ungetChar(c);
            return;
        }
        else
        {
            d_response.append(c);
            QByteArray numBytes = p_socket->read(numHeaderBytes);
            d_binaryDataLength = numBytes.toInt(&ok);
            if(!ok)
            {
                //something went wrong; perhaps we're in the midst of some binary data or something. Ignore this and look for a new hash
                d_headerHashFound = false;
                d_binaryDataLength = -1;
                d_response.clear();
                return;
            }
            d_response.append(numBytes);
        }
    }

    if(d_binaryDataLength > 0 && p_socket->bytesAvailable() >= d_binaryDataLength) // data are ready to be read in
    {
        //read it all, and reset acquisition parameters
        d_response.append(p_socket->readAll());
        d_headerHashFound = false;
        d_binaryDataLength = -1;
        d_waitingForReply = false;


        //copy the waveform prefix and append the response to it. Send to scanmanager for further processing
        emit fidAcquired(QByteArray(d_waveformPrefix).append(d_response));

//        emit logMessage(QString("Read binary data (total length: %2). Time elapsed: %1 ms").arg(timeTest.elapsed()).arg(d_response.size()));

        //if the user requested to change the resolution while we were acquiring, do it now
        if(d_resolutionChangePending)
            setResolution();


    }
//    else if(d_binaryDataLength > 0)
//        emit logMessage(QString("Waiting for full waveform.... (%1/%2) Time elapsed: %3 ms").arg(p_socket->bytesAvailable()).arg(d_binaryDataLength+2).arg(timeTest.elapsed()));

}

void DPO3012::setResolution()
{

    //wait until the instrument isn't trying to respond to a curve query
    if(d_waitingForReply)
    {
        d_resolutionChangePending = true;
        return;
    }

    //don't want this being intercepted by the replyReceived function
    disconnect(p_socket,&QIODevice::readyRead,this,&DPO3012::replyReceived);

    //get current resolution setting
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QtFTM::ScopeResolution r = (QtFTM::ScopeResolution)s.value(QString("%1/%2/resolution")
												   .arg(d_key).arg(d_subKey),(int)QtFTM::Res_5kHz).toInt();

    //apply appropriate settings. 10 kHz = 100 us, 5 kHz = 200 us, 2 kHz = 500 us, 1 kHz = 1000 us
    switch(r)
    {
    case QtFTM::Res_1kHz:
        p_comm->writeCmd(QString(":HORIZONTAL:SCALE 100e-6;RECORDLENGTH 100000;:DATA:STOP 100000\n"));
        break;
    case QtFTM::Res_2kHz:
        p_comm->writeCmd(QString(":HORIZONTAL:SCALE 100e-6;RECORDLENGTH 100000;:DATA:STOP 50000\n"));
        break;
    case QtFTM::Res_10kHz:
        p_comm->writeCmd(QString(":HORIZONTAL:SCALE 20e-6;RECORDLENGTH 10000;:DATA:STOP 5000\n"));
        break;
    case QtFTM::Res_5kHz:
    default:
        p_comm->writeCmd(QString(":HORIZONTAL:SCALE 20e-6;RECORDLENGTH 10000;:DATA:STOP 10000\n"));
        break;
    }

    if(p_socket->bytesAvailable())
        p_socket->readAll();

    //KLUDGE! Pause for a bit so that those commands have time to process. The scope might be triggering during this time
    p_socket->waitForReadyRead(100);
    p_socket->waitForReadyRead(100);

    //force a trigger event to make absolutely certain the settings update
    p_comm->writeCmd(QString("TRIGGER FORCE\n"));

    //wait again to make sure that has time to process before getting waevform prefix
    p_socket->waitForReadyRead(100);
    p_socket->waitForReadyRead(100);

    //get waveform prefix for use in parsing function
    d_waveformPrefix = p_comm->queryCmd(QString("WFMOUTPRE?\n"));

    //restart everything
    d_resolutionChangePending = false;
    connect(p_socket,&QIODevice::readyRead,this,&DPO3012::replyReceived,Qt::UniqueConnection);
}

void DPO3012::sendCurveQuery()
{
    if(d_waitingForWakeUp)
        return;

    //occasionally, oscilloscope just doesn't respond to curve query. This will (try to) restart it.
    if(d_acquisitionActive && d_waitingForReply && d_responseTimeout.elapsed() > 1000)// && d_response.isEmpty())
    {
        d_waitingForReply = false;
//        p_socket->write("DCL\n");
        emit logMessage(QString("Scope didn't reply. Attempting to restart in 30 seconds."),QtFTM::LogWarning);
        emit statusMessage(QString("Scope fell asleep. Will attempt to restart at %1").arg(QDateTime::currentDateTime().addSecs(30).toString()));
       QTimer::singleShot(30000,this,&DPO3012::wakeTheFUp);
        setActive(false);
        d_waitingForWakeUp = true;
        return;
    }

    if(d_acquisitionActive && !d_waitingForReply)
    {
        if(p_socket->bytesAvailable())
            p_socket->readAll();
        d_response.clear();
        p_comm->writeCmd(QString("CURVE?\n"));
        d_responseTimeout.restart();
        d_waitingForReply = true;
        d_headerHashFound = false;
        d_binaryDataLength = -1;
    }
}

void DPO3012::wakeTheFUp()
{
    //sometimes, the scope just stops responding. This is an effort to wake it the ^@#$ up!
    //testConnection sets d_waitingForWakeUp to false!
    if(!testConnection())
    {
        emit logMessage(QString("Scope stopped responding."),QtFTM::LogError);
        emit hardwareFailure();
        return;
    }
}

