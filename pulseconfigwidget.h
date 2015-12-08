#ifndef PULSECONFIGWIDGET_H
#define PULSECONFIGWIDGET_H

#include <QWidget>
#include <QList>

#include "pulsegenconfig.h"

class QLabel;
class QDoubleSpinBox;
class QPushButton;
class QToolButton;
class QLineEdit;
class QRadioButton;

namespace Ui {
class PulseConfigWidget;
}

class PulseConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PulseConfigWidget(QWidget *parent = 0);
    ~PulseConfigWidget();

    struct ChWidgets {
        QLabel *label;
        QDoubleSpinBox *delayBox;
        QDoubleSpinBox *widthBox;
        QDoubleSpinBox *endBox;
        QPushButton *onButton;
        QToolButton *cfgButton;
        QLineEdit *nameEdit;
        QPushButton *levelButton;
        QDoubleSpinBox *delayStepBox;
        QDoubleSpinBox *widthStepBox;
        QDoubleSpinBox *endStepBox;
        QRadioButton *widthButton;
        QRadioButton *endButton;
        bool endActive;

        ChWidgets() : endActive(false) {}
    };

    PulseGenConfig getConfig();
    int protDelay();
    int scopeDelay();

    //use when this is a display widget rather than a control widget (eg in single scan widget)
    void makeInternalConnections();



signals:
    void changeSetting(int,QtFTM::PulseSetting,QVariant);
    void changeRepRate(double);
    void changeProtDelay(int);
    void changeScopeDelay(int);

public slots:
    void launchChannelConfig(int ch);
    void newSetting(int index,QtFTM::PulseSetting s,QVariant val);
    void newConfig(const PulseGenConfig c);
    void changeDelay(int i, double d);
    void changeWidth(int i, double w);
    void changeEnd(int i, double e);
    void newRepRate(double r);
    void newProtDelay(int d);
    void newScopeDelay(int d);

private:
    Ui::PulseConfigWidget *ui;

    QList<ChWidgets> d_widgetList;
    double d_minWidth;
    double d_maxWidth;
    double d_minDelay;
    double d_maxDelay;


};

#endif // PULSECONFIGWIDGET_H
