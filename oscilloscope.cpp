#include "oscilloscope.h"
#include <QTimer>
#include <QFile>
#include <QStringList>
#include <math.h>

Oscilloscope::Oscilloscope(QObject *parent) :
    TcpInstrument(QString("scope"),QString("Oscilloscope"),parent), d_resolutionChangePending(false), d_acquisitionActive(false), d_waitingForReply(false), d_waitingForWakeUp(false)
{
}

Fid Oscilloscope::parseWaveform(const QByteArray d, double probeFreq)
{
    //the byte array consists of a waveform prefix with data about the scaling, etc, followed by a block of data preceded by #xyyyy (number of y characters = x)
    //to parse, first find the hash and split the prefix from the total array, then split the prefix on semicolons
    int hashIndex = d.indexOf('#');
    QByteArray prefix = d.mid(0,hashIndex);
    QList<QByteArray> prefixFields = prefix.split(';');

    //important data from prefix: number of bytes (0), byte format (3), byte order (4), x increment (10), y multiplier (14), y offset (15)
    if(prefixFields.size()<16)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Too few fields. If this problem persists, restart program."),LogHandler::Warning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    bool ok = true;
    int n_bytes = prefixFields.at(0).trimmed().toInt(&ok);
    if(!ok || n_bytes < 1 || n_bytes > 2)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid number of bytes per record. If this problem persists, restart program."),LogHandler::Warning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    bool n_signed = true;
    if(prefixFields.at(3).trimmed() == QByteArray("RP"))
        n_signed = false;

    QDataStream::ByteOrder n_order = QDataStream::BigEndian;
    if(prefixFields.at(4).trimmed() == QByteArray("LSB"))
        n_order = QDataStream::LittleEndian;

    double xIncr = prefixFields.at(10).trimmed().toDouble(&ok);
    if(!ok || xIncr <= 0.0)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid X spacing. If this problem persists, restart program."),LogHandler::Warning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    double yMult = prefixFields.at(14).trimmed().toDouble(&ok);
    if(!ok || yMult == 0.0)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid Y multipier. If this problem persists, restart program."),LogHandler::Warning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    double yOffset = prefixFields.at(14).trimmed().toDouble(&ok);
    if(!ok)
    {
//        emit logMessage(QString("Could not parse waveform prefix. Invalid Y offset. If this problem persists, restart program."),LogHandler::Warning);
        return Fid(5e-7,probeFreq,QVector<double>(400));
    }

    //calculate data stride
    int stride = (int)ceil(500e-9/xIncr);

    //now, locate and extract data block
    int numHeaderBytes = d.mid(hashIndex+1,1).toInt();
    int numDataBytes = d.mid(hashIndex+2,numHeaderBytes).toInt();
    int numRecords = numDataBytes/n_bytes;
    QByteArray dataBlock = d.mid(hashIndex+numHeaderBytes+2,numDataBytes);

    if(dataBlock.size() < numDataBytes)
    {
//        emit logMessage(QString("Could not parse waveform. Incomplete wave. If this problem persists, restart program."),LogHandler::Warning);
        return Fid(xIncr*(double)stride,probeFreq,QVector<double>(400));
    }

    //prepare data stream and data vector
    QDataStream ds(&dataBlock,QIODevice::ReadOnly);
    ds.setByteOrder(n_order);
    QVector<double> dat;
    dat.reserve((int)ceil(numRecords/stride));

    for(int i=0; i<numRecords; i++)
    {
        double yVal;
        if(n_bytes == 1)
        {
            if(n_signed)
            {
                qint8 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
            else
            {
                quint8 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
        }
        else
        {
            if(n_signed)
            {
                qint16 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
            else
            {
                quint16 num;
                ds >> num;
                yVal = yMult*((double)num+yOffset);
            }
        }
        if(!(i%stride))
            dat.append(yVal);
    }

    return Fid(xIncr*(double)stride,probeFreq,dat);

}

Oscilloscope::~Oscilloscope()
{
    if(d_socket->state() == QAbstractSocket::ConnectedState)
        writeCmd(QString("UNLOCK ALL\n"));
}

void Oscilloscope::initialize()
{
    TcpInstrument::initialize();

    setReadOptions(1000,true,QByteArray("\n\r"));

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.setValue(key().append(QString("/prettyName")),name());

	testConnection();
}

bool Oscilloscope::testConnection()
{
    d_waitingForWakeUp = false;
    setActive(false);
    disconnect(d_socket,&QIODevice::readyRead,this,&Oscilloscope::replyReceived);

    if(!TcpInstrument::testConnection())
    {
        emit connected(false,QString("TCP Socket error"));
        return false;
    }


    if(d_socket->bytesAvailable())
        d_socket->readAll();

    //send ID query
    d_socket->write("DCL\n");
    QByteArray resp = queryCmd(QString("*IDN?\n")).trimmed();
    if(resp.isEmpty())
    {
        emit connected(false,QString("%1 gave a null response to ID query").arg(d_prettyName));
        return false;
    }

    if(!resp.startsWith("TEKTRONIX,DPO3012"))
    {
        emit connected(false,QString("%1 response invalid. Received: %2").arg(d_prettyName).arg(QString(resp)));
        return false;
    }

    emit logMessage(QString("%1 ID response: %2").arg(d_prettyName).arg(QString(resp)));

    configure();

    if(d_socket->bytesAvailable())
        d_socket->readAll();

    //initialize acquisition variables
    d_headerHashFound = false;
    d_binaryDataLength = -1;

    //set resolution
    setResolution();

    setActive(true);

    connect(d_socket,&QIODevice::readyRead,this,&Oscilloscope::replyReceived,Qt::UniqueConnection);

    emit connected();
    return true;
}

void Oscilloscope::configure()
{
    //lock front panel
    writeCmd(QString("LOCK ALL;DCL\n"));

    //make appropriate settings

    //first, stop acquisition
    writeCmd(QString("ACQUIRE:STATE OFF\n"));

    //use channel 1 at full bandwidth, AC coupled, 125 mV/div (1 V full scale), units in V, offset 0
    writeCmd(QString("CH1:BANDWIDTH FULL;COUPLING AC;OFFSET 0;SCALE 0.125;YUNITS 'V'\n"));

    //triggering settings
    writeCmd(QString("TRIGGER:A:TYPE EDGE;MODE NORMAL;LEVEL TTL;HOLDOFF:TIME 50e-6\n"));
    writeCmd(QString("TRIGGER:A:EDGE:COUPLING DC;SLOPE FALL;SOURCE AUX\n"));

    //horizontal settings (except those pertaining to resolution)
    writeCmd(QString("HORIZONTAL:DELAY:MODE OFF;:HORIZONTAL:POSITION 0\n"));

    //data transfer settings
    writeCmd(QString("DATA:SOURCE CH1;START 1;WIDTH 2;ENCDG BIN\n"));

    //waveform transfer settings
    writeCmd(QString("WFMOUTPRE:ENCDG BIN;BN_FMT RI;BYT_OR MSB;BYT_NR 2\n"));

    //acquitision settings
    writeCmd(QString("ACQUIRE:MODE HIRES;STOPAFTER RUNSTOP\n"));

    //begin acquisition
    writeCmd(QString("ACQUIRE:STATE ON\n"));

    if(d_socket->bytesAvailable())
        d_socket->readAll();
}

void Oscilloscope::replyReceived()
{
    //this function is called when bytes are available at the TCP socket.
    //The scope manual describes the response format in more detail, but briefly, it looks like #xyyyy<data>\n\r
    //where x is an integer 1-9 that tells how many y digits follow, and yyyy is the number of bytes of binary data that follow before the \n\r

    //first step is to find the header hash (#)
    if(!d_headerHashFound)
    {
//        emit logMessage(QString("Looking for hash... (%1 ms)").arg(timeTest.elapsed()));

        //while there are bytes that can be read, read them and look for a hash
        while(d_socket->bytesAvailable()>0) // we can ignore anything that is before the hash
        {
            char c;
            d_socket->getChar(&c);
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
        d_socket->getChar(&c);
        bool ok = true;
        int numHeaderBytes = QByteArray(&c,1).toInt(&ok);
        if(!ok)
        {
            //something went wrong; perhaps we're in the midst of some binary data or something. Ignore this and look for a new hash
            d_headerHashFound = false;
            d_response.clear();
            return;
        }

        if(numHeaderBytes > d_socket->bytesAvailable()) // we don't have enough bytes! put that character back, and we'll try again when more data comes in
        {
            d_socket->ungetChar(c);
            return;
        }
        else
        {
            d_response.append(c);
            QByteArray numBytes = d_socket->read(numHeaderBytes);
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

    if(d_binaryDataLength > 0 && d_socket->bytesAvailable() >= d_binaryDataLength) // data are ready to be read in
    {
        //read it all, and reset acquisition parameters
        d_response.append(d_socket->readAll());
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
//        emit logMessage(QString("Waiting for full waveform.... (%1/%2) Time elapsed: %3 ms").arg(d_socket->bytesAvailable()).arg(d_binaryDataLength+2).arg(timeTest.elapsed()));

}

void Oscilloscope::setResolution()
{

    //wait until the instrument isn't trying to respond to a curve query
    if(d_waitingForReply)
    {
        d_resolutionChangePending = true;
        return;
    }

    //don't want this being intercepted by the replyReceived function
    disconnect(d_socket,&QIODevice::readyRead,this,&Oscilloscope::replyReceived);

    //get current resolution setting
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    Resolution r = (Resolution)s.value(QString("%1/resolution").arg(d_key),(int)Res_5kHz).toInt();

    //apply appropriate settings. 10 kHz = 100 us, 5 kHz = 200 us, 2 kHz = 500 us, 1 kHz = 1000 us
    switch(r)
    {
    case Res_1kHz:
        writeCmd(QString(":HORIZONTAL:SCALE 100e-6;RECORDLENGTH 100000;:DATA:STOP 100000\n"));
        break;
    case Res_2kHz:
        writeCmd(QString(":HORIZONTAL:SCALE 100e-6;RECORDLENGTH 100000;:DATA:STOP 50000\n"));
        break;
    case Res_10kHz:
        writeCmd(QString(":HORIZONTAL:SCALE 20e-6;RECORDLENGTH 10000;:DATA:STOP 5000\n"));
        break;
    case Res_5kHz:
    default:
        writeCmd(QString(":HORIZONTAL:SCALE 20e-6;RECORDLENGTH 10000;:DATA:STOP 10000\n"));
        break;
    }

    if(d_socket->bytesAvailable())
        d_socket->readAll();

    //KLUDGE! Pause for a bit so that those commands have time to process. The scope might be triggering during this time
    d_socket->waitForReadyRead(100);
    d_socket->waitForReadyRead(100);

    //force a trigger event to make absolutely certain the settings update
    writeCmd(QString("TRIGGER FORCE\n"));

    //wait again to make sure that has time to process before getting waevform prefix
    d_socket->waitForReadyRead(100);
    d_socket->waitForReadyRead(100);

    //get waveform prefix for use in parsing function
    d_waveformPrefix = queryCmd(QString("WFMOUTPRE?\n"));

    //restart everything
    d_resolutionChangePending = false;
    connect(d_socket,&QIODevice::readyRead,this,&Oscilloscope::replyReceived,Qt::UniqueConnection);
}

void Oscilloscope::sendCurveQuery()
{
    if(d_waitingForWakeUp)
        return;

    //occasionally, oscilloscope just doesn't respond to curve query. This will (try to) restart it.
    if(d_acquisitionActive && d_waitingForReply && d_responseTimeout.elapsed() > 1000)// && d_response.isEmpty())
    {
        d_waitingForReply = false;
//        d_socket->write("DCL\n");
        emit logMessage(QString("Scope didn't reply. Attempting to restart in 30 seconds."),LogHandler::Warning);
        emit statusMessage(QString("Scope fell asleep. Will attempt to restart at %1").arg(QDateTime::currentDateTime().addSecs(30).toString()));
	   QTimer::singleShot(30000,this,&Oscilloscope::wakeTheFUp);
        setActive(false);
        d_waitingForWakeUp = true;
        return;
    }

    if(d_acquisitionActive && !d_waitingForReply)
    {
        if(d_socket->bytesAvailable())
            d_socket->readAll();
        d_response.clear();
        writeCmd(QString("CURVE?\n"));
        d_responseTimeout.restart();
        d_waitingForReply = true;
        d_headerHashFound = false;
        d_binaryDataLength = -1;
    }
}

void Oscilloscope::wakeTheFUp()
{
    //sometimes, the scope just stops responding. This is an effort to wake it the ^@#$ up!
    //testConnection sets d_waitingForWakeUp to false!
    if(!testConnection())
    {
        emit logMessage(QString("Scope stopped responding."),LogHandler::Error);
        emit hardwareFailure();
        return;
    }
}
