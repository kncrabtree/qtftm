#include "amdorplot.h"

#include <QPointF>
#include <QPair>
#include <QSettings>
#include <QEvent>
#include <QMouseEvent>
#include <QColorDialog>
#include <QMenu>
#include <QComboBox>
#include <QWidgetAction>
#include <QFormLayout>
#include <QSpinBox>

#include <qwt6/qwt_plot_curve.h>
#include <qwt6/qwt_symbol.h>
#include <qwt6/qwt_legend.h>

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

    QwtLegend *l = new QwtLegend;
    l->setDefaultItemMode(QwtLegendData::Checkable);
    l->installEventFilter(this);
    connect(l,&QwtLegend::checked,this,&AmdorPlot::toggleCurveVisibility);
    insertLegend(l);

    p_diagCurve = nullptr;

}

void AmdorPlot::setPlotRange(double min, double max)
{
    //add an extra GHz on either side to be safe
    setAxisAutoScaleRange(QwtPlot::yLeft,min-1000.0,max+1000.0);
    setAxisAutoScaleRange(QwtPlot::xBottom,min-1000.0,max+1000.0);

    if(p_diagCurve == nullptr)
    {
        p_diagCurve = new QwtPlotCurve;
        p_diagCurve->setItemAttribute(QwtPlotItem::Legend,false);
        p_diagCurve->setRenderHint(QwtPlotCurve::RenderAntialiased);
        QPen p(QPalette().color(QPalette::Text));
        p.setStyle(Qt::DashLine);
        p_diagCurve->setPen(p);
        p_diagCurve->attach(this);
    }

    QVector<QPointF> dDat { QPointF(min-1000.0,min-1000.0), QPointF(max+1000.0,max+1000.0) };
    p_diagCurve->setSamples(dDat);
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
        while(d_curves.size() < i+1)
        {
            QwtPlotCurve *c = new QwtPlotCurve(QString("Set %1").arg(i));
            if(i == 0)
                c->setTitle(QString("No link"));

            c->setStyle(QwtPlotCurve::NoCurve);
            c->setRenderHint(QwtPlotCurve::RenderAntialiased);

            QwtSymbol::Style style = static_cast<QwtSymbol::Style>(s.value(QString("symbol"),static_cast<int>(QwtSymbol::Ellipse)).toInt());
            QColor color = s.value(QString("color"),QPalette().color(QPalette::Text)).value<QColor>();
            int size = s.value(QString("size"),5).toInt();

            QwtSymbol *sym = new QwtSymbol(style);
            sym->setColor(color);
            sym->setPen(QPen(color));
            sym->setSize(size);
            c->setSymbol(sym);

            c->setLegendAttribute(QwtPlotCurve::LegendShowSymbol);
            c->attach(this);
            d_curves.append(c);

        }

        d_curves[i]->setSamples(l.at(i));
    }
    s.endArray();
    s.endGroup();

    for(int i=d_curves.size()-1; i>=l.size(); i--)
    {
        QwtPlotCurve *c = d_curves.takeAt(i);
        c->detach();
        delete c;
    }

    replot();
}

void AmdorPlot::toggleCurveVisibility(QVariant item, bool hide, int index)
{
    Q_UNUSED(index)
    QwtPlotCurve *c = dynamic_cast<QwtPlotCurve*>(infoToItem(item));
    if(c)
        c->setVisible(!hide);

    replot();
}

void AmdorPlot::changeCurveColor(QwtPlotCurve *c, QColor currentColor)
{
    if(c == nullptr)
        return;

    int index = d_curves.indexOf(c);
    if(index < 0)
        return;

    QColor color = QColorDialog::getColor(currentColor);
    if(color.isValid())
    {
        QSettings s;
        s.beginGroup(QString("amdorPlot"));
        s.beginWriteArray(QString("set"));
        s.setArrayIndex(index);
        QwtSymbol *sym = new QwtSymbol(c->symbol()->style());
        sym->setColor(color);
        sym->setPen(QPen(color));
        sym->setSize(c->symbol()->size());
        c->setSymbol(sym);
        s.setValue(QString("color"),color);
        replot();
        s.endArray();
        s.endGroup();
        s.sync();
    }
}

void AmdorPlot::changeCurveSymbol(QwtPlotCurve *c, QwtSymbol::Style sty)
{
    if(c == nullptr)
        return;

    int index = d_curves.indexOf(c);
    if(index < 0)
        return;


    QSettings s;
    s.beginGroup(QString("amdorPlot"));
    s.beginWriteArray(QString("set"));
    s.setArrayIndex(index);
    QColor currentColor = s.value(QString("color"),QPalette().color(QPalette::Text)).value<QColor>();
    QwtSymbol *sym = new QwtSymbol(sty);
    sym->setColor(currentColor);
    sym->setPen(QPen(currentColor));
    sym->setSize(c->symbol()->size());
    c->setSymbol(sym);
    s.setValue(QString("symbol"),static_cast<int>(sty));
    replot();
    s.endArray();
    s.endGroup();
    s.sync();
}

void AmdorPlot::changeCurveSize(QwtPlotCurve *c, int size)
{
    if(c == nullptr)
        return;

    int index = d_curves.indexOf(c);
    if(index < 0)
        return;


    QSettings s;
    s.beginGroup(QString("amdorPlot"));
    s.beginWriteArray(QString("set"));
    s.setArrayIndex(index);
    QColor currentColor = s.value(QString("color"),QPalette().color(QPalette::Text)).value<QColor>();
    QwtSymbol *sym = new QwtSymbol(c->symbol()->style());
    sym->setColor(currentColor);
    sym->setPen(QPen(currentColor));
    sym->setSize(size);
    c->setSymbol(sym);
    s.setValue(QString("size"),size);
    replot();
    s.endArray();
    s.endGroup();
    s.sync();
}



void AmdorPlot::filterData()
{
    //no filtering needed
}

QMenu* AmdorPlot::curveContextMenu(QwtPlotCurve *c)
{
    if(c == nullptr)
        return nullptr;

    int index = d_curves.indexOf(c);
    if(index < 0)
        return nullptr;

    QSettings s;
    s.beginGroup(QString("amdorPlot"));
    s.beginWriteArray(QString("set"));
    s.setArrayIndex(index);
    QColor currentColor = s.value(QString("color"),QPalette().color(QPalette::Text)).value<QColor>();
    int currentSymbol = static_cast<int>(c->symbol()->style());
    int currentSize = c->symbol()->size().width();
    s.endArray();
    s.endGroup();

    QMenu *out = new QMenu;
    connect(out,&QMenu::aboutToHide,out,&QMenu::deleteLater);

    QAction *colorAction = out->addAction(QString("Change color..."));
    connect(colorAction,&QAction::triggered,[=](){ changeCurveColor(c,currentColor); });


    QComboBox *symBox = new QComboBox(out);
    symBox->addItem(QString::fromUtf16(u"●"),(int)QwtSymbol::Ellipse);
    symBox->addItem(QString::fromUtf16(u"◾"),(int)QwtSymbol::Rect);
    symBox->addItem(QString::fromUtf16(u"◆"),(int)QwtSymbol::Diamond);
    symBox->addItem(QString::fromUtf16(u"▲"),(int)QwtSymbol::Triangle);
    symBox->addItem(QString::fromUtf16(u"◀"),(int)QwtSymbol::LTriangle);
    symBox->addItem(QString::fromUtf16(u"▶"),(int)QwtSymbol::RTriangle);
    symBox->addItem(QString::fromUtf16(u"▼"),(int)QwtSymbol::DTriangle);
    symBox->addItem(QString::fromUtf16(u"+"),(int)QwtSymbol::Cross);
    symBox->addItem(QString::fromUtf16(u"⨯"),(int)QwtSymbol::XCross);
    symBox->addItem(QString::fromUtf16(u"—"),(int)QwtSymbol::HLine);
    symBox->addItem(QString::fromUtf16(u"|"),(int)QwtSymbol::VLine);
    symBox->addItem(QString::fromUtf16(u"✳"),(int)QwtSymbol::Star1);
    symBox->addItem(QString::fromUtf16(u"✶"),(int)QwtSymbol::Star2);
    symBox->addItem(QString::fromUtf16(u"⬢"),(int)QwtSymbol::Hexagon);

    symBox->setCurrentIndex(symBox->findData(currentSymbol));
    connect(symBox,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),[=](int i){ changeCurveSymbol(c,static_cast<QwtSymbol::Style>(symBox->itemData(i).toInt())); });

    QSpinBox *sizeBox = new QSpinBox(out);
    sizeBox->setRange(1,10);
    sizeBox->setValue(currentSize);
    connect(sizeBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),[=](int s) { changeCurveSize(c,s);});

    QWidgetAction *wa = new QWidgetAction(out);
    QWidget *w = new QWidget(out);
    QFormLayout *fl = new QFormLayout;
    fl->addRow(QString("Symbol"),symBox);
    fl->addRow(QString("Size"),sizeBox);
    w->setLayout(fl);
    wa->setDefaultWidget(w);
    out->addAction(wa);

    return out;
}


bool AmdorPlot::eventFilter(QObject *obj, QEvent *ev)
{
    if(obj == static_cast<QObject*>(legend()))
    {
        if(ev->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *me = static_cast<QMouseEvent*>(ev);
            if(me && me->button() == Qt::RightButton)
            {
                //here's the hook for changing color
                QwtLegend *l = static_cast<QwtLegend*>(legend());
                QVariant item = l->itemInfo(l->contentsWidget()->childAt(me->pos()));
                QwtPlotCurve *c = dynamic_cast<QwtPlotCurve*>(infoToItem(item));
                if(c)
                {
                    ev->accept();
                    QMenu *m = curveContextMenu(c);
                    if(m != nullptr)
                        m->popup(me->globalPos());
                }
                else
                    ev->ignore();

                return true;
            }
        }
    }
    return ZoomPanPlot::eventFilter(obj,ev);
}
