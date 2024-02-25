#include "dopplerpair.h"
#include "qcolor.h"
#include "qpen.h"
#include <QFontMetrics>
#include <QTextStream>
#include <qwt6/qwt_text.h>

DopplerPair::DopplerPair(int n, double c, double s, double a, QObject *parent) :
    QObject(parent), num(n), d_center(c), d_splitting(s), d_amplitude(a)
{
	//configure marker style
	redMarker.setLineStyle(QwtPlotMarker::VLine);
	blueMarker.setLineStyle(QwtPlotMarker::VLine);
	redMarker.setLinePen(QPen(QColor(Qt::red),1.0));
	blueMarker.setLinePen(QPen(QColor(Qt::blue),1.0));

	//configure marker label style
	QFont f(QString("sans serif"),8);
	QString text = makeMarkerText();
	QwtText rText(text);
	rText.setColor(QColor(Qt::red));
	rText.setFont(f);
	redMarker.setLabel(rText);
	redMarker.setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);

	QwtText bText(text);
	bText.setColor(QColor(Qt::blue));
	bText.setFont(f);
	blueMarker.setLabel(bText);
	blueMarker.setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);

	//set the x values of the markers
	updateMarkers();

}

const QString DopplerPair::toString() const
{
	return QString::fromUtf8("%1: %2 ± %3 MHz").arg(num).arg(d_center,0,'f',4).arg(d_splitting,0,'f',4);
}

void DopplerPair::attach(QwtPlot *p)
{
	//attach markers to plot
	redMarker.attach(p);
	blueMarker.attach(p);
}

void DopplerPair::detach()
{
	//detach markers from plot
	redMarker.detach();
	blueMarker.detach();
}

void DopplerPair::moveCenter(double newPos)
{
	d_center = newPos;
	updateMarkers();
}

void DopplerPair::split(double newSplit)
{
	d_splitting = newSplit;
	updateMarkers();
}

void DopplerPair::changeNumber(int newNum)
{
	num = newNum;
	QString text = makeMarkerText();

	QwtText rText = redMarker.label();
	rText.setText(text);
	redMarker.setLabel(rText);

	QwtText bText = blueMarker.label();
	bText.setText(text);
	blueMarker.setLabel(bText);

}

void DopplerPair::updateMarkers()
{
	redMarker.setXValue(d_center - d_splitting);
	blueMarker.setXValue(d_center + d_splitting);
}

QString DopplerPair::makeMarkerText()
{
	//to make sure numbers don't overlap, we add newlines equal to the scan number (number 3 has 3 newlines)
	QString out;
	for(int i=0; i<num; i++)
		out.append(QString("\n"));
	out.append(QString("%1").arg(num));
	return out;
}

