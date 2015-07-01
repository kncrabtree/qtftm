#ifndef ATTENUATOR_H
#define ATTENUATOR_H

#include "tcpinstrument.h"
#include <QFile>

class Attenuator : public TcpInstrument
{
	Q_OBJECT
public:
	explicit Attenuator(QObject *parent = nullptr);
	
signals:
	void attnUpdate(int);
    void taattnUpdate(int);
    void attenFileParseSuccess(bool);


public slots:
	void initialize();
	bool testConnection();
    void changeAttenFile(QString fileName);

    int setTuningAttn(double freq);
	int setAttn(int a);
	int readAttn();
    void clearAttenData();

private:
    QList<QPair<double,int> > d_attenData;
    bool parseAttenFile(QString fileName);
	
};

#endif // ATTENUATOR_H
