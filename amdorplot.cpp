#include "amdorplot.h"

#include <QPointF>
#include <QPair>
#include <QSettings>

#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_symbol.h>

AmdorPlot::AmdorPlot(QWidget *parent) : ZoomPanPlot(QString("amdorPlot"),parent)
{
    QFont labelFont(QString("sans serif"),8);
    QwtText xlabel = QwtText(QString("F1 (MHz)"));
    QwtText ylabel = QwtText(QString("F2 (MHz)"));
    xlabel.setFont(labelFont);
    ylabel.setFont(labelFont);

    setAxisFont(QwtPlot::xBottom,labelFont);
    setAxisFont(QwtPlot::yLeft,labelFont);
    setAxisTitle(QwtPlot::yLeft,ylabel);
    setAxisTitle(QwtPlot::xBottom,xlabel);

}

void AmdorPlot::setPlotRange(double min, double max)
{
    //add an extra GHz on either side to be safe
    setAxisAutoScaleRange(QwtPlot::yLeft,min-1000.0,max+1000.0);
    setAxisAutoScaleRange(QwtPlot::xBottom,min-1000.0,max+1000.0);
    replot();
}

void AmdorPlot::updateData(QList<QVector<QPointF>> l)
{
    QSettings s;
    s.beginGroup(QString("amdorPlot"));
    s.beginReadArray(QString("set"));
    for(int i=0; i<l.size(); i++)
    {
        s.setArrayIndex(i);
        while(d_curves.size() <= i+1)
        {
            QwtPlotCurve *c = new QwtPlotCurve(QString("Set %1"));
            if(i == 0)
                c->setTitle(QString("No link"));

            c->setStyle(QwtPlotCurve::NoCurve);
            c->setRenderHint(QwtPlotCurve::RenderAntialiased);

            QwtSymbol::Style style = static_cast<QwtSymbol::Style>(s.value(QString("symbol"),static_cast<int>(QwtSymbol::Ellipse)).toInt());
            QColor color = s.value(QString("color"),QPalette().color(QPalette::Text)).value<QColor>();

            QwtSymbol *sym = new QwtSymbol(style);
            sym->setColor(color);
            sym->setSize(5);
            c->setSymbol(sym);

            c->setLegendAttribute(QwtPlotCurve::LegendShowSymbol);
            c->attach(this);
            d_curves.append(c);

        }

        d_curves[i]->setSamples(l.at(i));
    }
    s.endArray();
    s.endGroup();

    replot();
}



void AmdorPlot::filterData()
{
    //no filtering needed
}
