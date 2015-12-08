#include "pulseconfigwidget.h"
#include "ui_pulseconfigwidget.h"

#include <QSettings>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QRadioButton>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>

PulseConfigWidget::PulseConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PulseConfigWidget)
{
    ui->setupUi(this);    

    auto vc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    auto ivc = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("pulseGenerator"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());

    d_minWidth = s.value(QString("minWidth"),0.004).toDouble();
    d_maxWidth = s.value(QString("maxWidth"),100000.0).toDouble();
    d_minDelay = s.value(QString("minDelay"),0.0).toDouble();
    d_maxDelay = s.value(QString("maxDelay"),100000.0).toDouble();

    s.beginReadArray(QString("channels"));
    for(int i=0; i<QTFTM_PGEN_NUMCHANNELS; i++)
    {
        s.setArrayIndex(i);
        ChWidgets ch;
        int col = 0;

        ch.label = new QLabel(s.value(QString("name"),QString("Ch%1").arg(i)).toString(),this);
	   ch.label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        ui->pulseConfigBoxLayout->addWidget(ch.label,i+1,col,1,1);
        col++;

        ch.delayBox = new QDoubleSpinBox(this);
        ch.delayBox->setKeyboardTracking(false);
       ch.delayBox->setRange(d_minDelay,d_maxDelay);
        ch.delayBox->setDecimals(3);
        ch.delayBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.delayBox->setValue(s.value(QString("defaultDelay"),0.0).toDouble());
        ch.delayBox->setSingleStep(s.value(QString("delayStep"),1.0).toDouble());
        ui->pulseConfigBoxLayout->addWidget(ch.delayBox,i+1,col,1,1);
        connect(ch.delayBox,vc,this,[=](double val){ changeDelay(i,val); } );
        col++;

        ch.endActive = s.value(QString("endActive"),false).toBool();


        ch.widthBox = new QDoubleSpinBox(this);
        ch.widthBox->setKeyboardTracking(false);
       ch.widthBox->setRange(d_minWidth,d_maxWidth);
        ch.widthBox->setDecimals(3);
        ch.widthBox->setSuffix(QString::fromUtf16(u" µs (Width)"));
        ch.widthBox->setValue(s.value(QString("defaultWidth"),0.050).toDouble());
        ch.widthBox->setSingleStep(s.value(QString("widthStep"),1.0).toDouble());
        if(ch.endActive)
            ch.widthBox->hide();
        else
            ui->pulseConfigBoxLayout->addWidget(ch.widthBox,i+1,col,1,1);
        connect(ch.widthBox,vc,this,[=](double val){ changeWidth(i,val); } );

        ch.endBox = new QDoubleSpinBox(this);
        ch.endBox->setKeyboardTracking(false);
        ch.endBox->setRange(ch.delayBox->value()+d_minWidth, ch.delayBox->value()+d_maxWidth);
        ch.endBox->setDecimals(3);
        ch.endBox->setSuffix(QString::fromUtf16(u" µs (End)"));
        ch.endBox->setValue(s.value(QString("defaultDelay")).toDouble()+s.value(QString("defaultWidth"),0.050).toDouble());
        ch.endBox->setSingleStep(s.value(QString("endStep"),1.0).toDouble());
        if(ch.endActive)
            ui->pulseConfigBoxLayout->addWidget(ch.endBox,i+1,col,1,1);
        else
            ch.endBox->hide();
        connect(ch.endBox,vc,this,[=](double val){ changeEnd(i,val); } );
        col++;

        ch.onButton = new QPushButton(this);
        ch.onButton->setCheckable(true);
        ch.onButton->setChecked(s.value(QString("defaultEnabled"),false).toBool());
        if(ch.onButton->isChecked())
            ch.onButton->setText(QString("On"));
        else
            ch.onButton->setText(QString("Off"));
        ui->pulseConfigBoxLayout->addWidget(ch.onButton,i+1,col,1,1);
        connect(ch.onButton,&QPushButton::toggled,this,[=](bool en){ emit changeSetting(i,QtFTM::PulseEnabled,en); } );
        connect(ch.onButton,&QPushButton::toggled,this,[=](bool en){
            if(en)
                ch.onButton->setText(QString("On"));
            else
                ch.onButton->setText(QString("Off")); } );
        if(i == QTFTM_PGEN_GASCHANNEL || i == QTFTM_PGEN_MWCHANNEL)
            ch.onButton->setEnabled(false);

        col++;

        ch.cfgButton = new QToolButton(this);
        ch.cfgButton->setIcon(QIcon(":/icons/configure.png"));
        ui->pulseConfigBoxLayout->addWidget(ch.cfgButton,i+1,col,1,1);
        connect(ch.cfgButton,&QToolButton::clicked,[=](){ launchChannelConfig(i); } );
        col++;

        ch.nameEdit = new QLineEdit(ch.label->text(),this);
        ch.nameEdit->setMaxLength(8);
        ch.nameEdit->hide();

        ch.levelButton = new QPushButton(this);
        ch.levelButton->setCheckable(true);
        if(static_cast<QtFTM::PulseActiveLevel>(s.value(QString("level"),QtFTM::PulseLevelActiveHigh).toInt()) == QtFTM::PulseLevelActiveHigh)
        {
            ch.levelButton->setChecked(true);
            ch.levelButton->setText(QString("Active High"));
        }
        else
        {
            ch.levelButton->setChecked(false);
            ch.levelButton->setText(QString("Active Low"));
        }
        connect(ch.levelButton,&QPushButton::toggled,[=](bool en){
            if(en)
                ch.levelButton->setText(QString("Active High"));
            else
                ch.levelButton->setText(QString("Active Low"));
        });
	   if(i == QTFTM_PGEN_MWCHANNEL)
		  ch.levelButton->setEnabled(false);
        ch.levelButton->hide();

        ch.delayStepBox = new QDoubleSpinBox(this);
        ch.delayStepBox->setDecimals(3);
        ch.delayStepBox->setRange(0.001,1000.0);
        ch.delayStepBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.delayStepBox->setValue(s.value(QString("delayStep"),1.0).toDouble());
        ch.delayStepBox->hide();

        ch.widthStepBox = new QDoubleSpinBox(this);
        ch.widthStepBox->setDecimals(3);
        ch.widthStepBox->setRange(0.001,1000.0);
        ch.widthStepBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.widthStepBox->setValue(s.value(QString("widthStep"),1.0).toDouble());
        ch.widthStepBox->hide();

        ch.endStepBox = new QDoubleSpinBox(this);
        ch.endStepBox->setDecimals(3);
        ch.endStepBox->setRange(0.001,1000.0);
        ch.endStepBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.endStepBox->setValue(s.value(QString("endStep"),1.0).toDouble());
        ch.endStepBox->hide();

        ch.widthButton = new QRadioButton(QString("Delay/Width Mode"),this);
        ch.widthButton->setChecked(!ch.endActive);
        ch.widthButton->hide();

        ch.endButton = new QRadioButton(QString("Delay/End Mode"),this);
        ch.endButton->setChecked(ch.endActive);
        ch.endButton->hide();

        d_widgetList.append(ch);
    }

    ui->pulseConfigBoxLayout->setColumnStretch(0,0);
    ui->pulseConfigBoxLayout->setColumnStretch(1,1);
    ui->pulseConfigBoxLayout->setColumnStretch(2,1);
    ui->pulseConfigBoxLayout->setColumnStretch(3,0);
    ui->pulseConfigBoxLayout->setColumnStretch(4,0);

    connect(ui->repRateBox,vc,this,&PulseConfigWidget::changeRepRate);

    s.endArray();
    s.endGroup();
    s.endGroup();

    s.beginGroup(QString("pdg"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    ui->protDelayBox->setMinimum(s.value(QString("minProt"),1).toInt());
    ui->protDelayBox->setMaximum(s.value(QString("maxProt"),100).toInt());
    ui->scopeDelayBox->setMinimum(s.value(QString("minScope"),0).toInt());
    ui->scopeDelayBox->setMaximum(s.value(QString("maxScope"),100).toInt());
    s.endGroup();
    s.endGroup();

    ui->protDelayBox->setSuffix(QString::fromUtf16(u" μs"));
    ui->scopeDelayBox->setSuffix(QString::fromUtf16(u" μs"));

    connect(ui->protDelayBox,ivc,this,&PulseConfigWidget::changeProtDelay);
    connect(ui->scopeDelayBox,ivc,this,&PulseConfigWidget::changeScopeDelay);
}

PulseConfigWidget::~PulseConfigWidget()
{
    delete ui;
}

PulseGenConfig PulseConfigWidget::getConfig()
{
	return ui->pulsePlot->config();
}

int PulseConfigWidget::protDelay()
{
	return ui->protDelayBox->value();
}

int PulseConfigWidget::scopeDelay()
{
	return ui->scopeDelayBox->value();
}

void PulseConfigWidget::makeInternalConnections()
{
    ui->repRateBox->setEnabled(false);
    connect(this,&PulseConfigWidget::changeSetting,ui->pulsePlot,&PulsePlot::newSetting);
    connect(this,&PulseConfigWidget::changeProtDelay,ui->pulsePlot,&PulsePlot::newProtDelay);
    connect(this,&PulseConfigWidget::changeScopeDelay,ui->pulsePlot,&PulsePlot::newScopeDelay);
}

void PulseConfigWidget::launchChannelConfig(int ch)
{
    if(ch < 0 || ch >= d_widgetList.size())
        return;

    QDialog d(this);
    d.setWindowTitle(QString("Configure Pulse Channel %1").arg(ch));

    QFormLayout *fl = new QFormLayout();
    QVBoxLayout *vbl = new QVBoxLayout();
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb->button(QDialogButtonBox::Ok),&QPushButton::clicked,&d,&QDialog::accept);
    connect(bb->button(QDialogButtonBox::Cancel),&QPushButton::clicked,&d,&QDialog::reject);

    ChWidgets chw = d_widgetList[ch];

    fl->addRow(QString("Channel Name"),chw.nameEdit);
    fl->addRow(QString("Active Level"),chw.levelButton);
    fl->addRow(QString("Delay Step Size"),chw.delayStepBox);
    fl->addRow(QString("Width Step Size"),chw.widthStepBox);
    fl->addRow(QString("End Step Size"),chw.endStepBox);
    fl->addRow(chw.widthButton);
    fl->addRow(chw.endButton);

    if(chw.endActive)
    {
        chw.widthButton->setChecked(false);
        chw.endButton->setChecked(true);
    }
    else
    {
        chw.widthButton->setChecked(true);
        chw.endButton->setChecked(false);
    }

    chw.nameEdit->show();
    chw.levelButton->show();
    chw.delayStepBox->show();
    chw.widthStepBox->show();
    chw.endStepBox->show();
    chw.widthButton->show();
    chw.endButton->show();

    vbl->addLayout(fl,1);
    vbl->addWidget(bb);

    d.setLayout(vbl);
    if(d.exec() == QDialog::Accepted)
    {
        QSettings s(QSettings::SystemScope, QApplication::organizationName(), QApplication::applicationName());
	   QString subKey = s.value(QString("pulseGenerator/subKey"),QString("virtual")).toString();
	   s.beginGroup(QString("pulseGenerator"));;
        s.beginGroup(subKey);
        s.beginWriteArray(QString("channels"));
        s.setArrayIndex(ch);

        chw.label->setText(chw.nameEdit->text());
        s.setValue(QString("name"),chw.nameEdit->text());
        emit changeSetting(ch,QtFTM::PulseName,chw.nameEdit->text());

        chw.delayBox->setSingleStep(chw.delayStepBox->value());
        s.setValue(QString("delayStep"),chw.delayStepBox->value());

        chw.widthBox->setSingleStep(chw.widthStepBox->value());
        s.setValue(QString("widthStep"),chw.widthStepBox->value());

        chw.endBox->setSingleStep(chw.endStepBox->value());
        s.setValue(QString("endStep"),chw.endStepBox->value());

        if(chw.levelButton->isChecked())
        {
            s.setValue(QString("level"),QtFTM::PulseLevelActiveHigh);
            emit changeSetting(ch,QtFTM::PulseLevel,QVariant::fromValue(QtFTM::PulseLevelActiveHigh));
        }
        else
        {
            s.setValue(QString("level"),QtFTM::PulseLevelActiveLow);
            emit changeSetting(ch,QtFTM::PulseLevel,QVariant::fromValue(QtFTM::PulseLevelActiveLow));
        }

        if(chw.widthButton->isChecked() && chw.endActive)
        {
            ui->pulseConfigBoxLayout->replaceWidget(chw.endBox,chw.widthBox);
            chw.widthBox->show();
            chw.endBox->hide();
            chw.endActive = false;
        }
        else if(chw.endButton->isChecked() && !chw.endActive)
        {
            ui->pulseConfigBoxLayout->replaceWidget(chw.widthBox,chw.endBox);
            chw.endBox->show();
            chw.widthBox->hide();
            chw.endActive = true;
        }
        s.setValue(QString("endActive"),chw.endActive);
        d_widgetList[ch].endActive = chw.endActive;

        s.endArray();
        s.endGroup();
        s.endGroup();
        s.sync();
    }

    chw.nameEdit->setParent(this);
    chw.nameEdit->hide();
    chw.levelButton->setParent(this);
    chw.levelButton->hide();
    chw.delayStepBox->setParent(this);
    chw.delayStepBox->hide();
    chw.widthStepBox->setParent(this);
    chw.widthStepBox->hide();
    chw.endStepBox->setParent(this);
    chw.endStepBox->hide();
    chw.widthButton->setParent(this);
    chw.widthButton->hide();
    chw.endButton->setParent(this);
    chw.endButton->hide();

}

void PulseConfigWidget::newSetting(int index, QtFTM::PulseSetting s, QVariant val)
{
    if(index < 0 || index > d_widgetList.size())
        return;

    blockSignals(true);
    d_widgetList.at(index).delayBox->blockSignals(true);
    d_widgetList.at(index).widthBox->blockSignals(true);
    d_widgetList.at(index).endBox->blockSignals(true);
    d_widgetList.at(index).onButton->blockSignals(true);
    d_widgetList.at(index).levelButton->blockSignals(true);

    switch(s) {
    case QtFTM::PulseName:
        d_widgetList.at(index).label->setText(val.toString());
        d_widgetList.at(index).nameEdit->setText(val.toString());
        break;
    case QtFTM::PulseDelay:
        d_widgetList.at(index).delayBox->setValue(val.toDouble());
        d_widgetList.at(index).endBox->setRange(val.toDouble()+d_minWidth,val.toDouble()+d_maxWidth);
        if(d_widgetList.at(index).endActive)
            d_widgetList.at(index).widthBox->setValue(qAbs(d_widgetList.at(index).endBox->value() - val.toDouble()));
        else
            d_widgetList.at(index).endBox->setValue(qAbs(d_widgetList.at(index).widthBox->value() + val.toDouble()));
        break;
    case QtFTM::PulseWidth:
        d_widgetList.at(index).widthBox->setValue(val.toDouble());
        if(!d_widgetList.at(index).endActive)
            d_widgetList.at(index).endBox->setValue(val.toDouble() + d_widgetList.at(index).delayBox->value());
        break;
    case QtFTM::PulseLevel:
        d_widgetList.at(index).levelButton->setChecked(val == QVariant(QtFTM::PulseLevelActiveHigh));
        break;
    case QtFTM::PulseEnabled:
        d_widgetList.at(index).onButton->setChecked(val.toBool());
        break;
    }

    d_widgetList.at(index).delayBox->blockSignals(false);
    d_widgetList.at(index).widthBox->blockSignals(false);
    d_widgetList.at(index).endBox->blockSignals(false);
    d_widgetList.at(index).onButton->blockSignals(false);
    d_widgetList.at(index).levelButton->blockSignals(false);
    blockSignals(false);

    ui->pulsePlot->newSetting(index,s,val);
}

void PulseConfigWidget::newConfig(const PulseGenConfig c)
{
    blockSignals(true);
    for(int i=0; i<c.size(); i++)
    {
        d_widgetList.at(i).label->setText(c.at(i).channelName);
        d_widgetList.at(i).nameEdit->setText(c.at(i).channelName);
        d_widgetList.at(i).delayBox->setValue(c.at(i).delay);
        d_widgetList.at(i).widthBox->setValue(c.at(i).width);
        d_widgetList.at(i).endBox->setRange(c.at(i).delay + d_minWidth, c.at(i).delay + d_maxWidth);
        d_widgetList.at(i).endBox->setValue(c.at(i).delay + c.at(i).width);
        d_widgetList.at(i).levelButton->setChecked(c.at(i).level == QtFTM::PulseLevelActiveHigh);
        d_widgetList.at(i).onButton->setChecked(c.at(i).enabled);
    }
    ui->repRateBox->setValue(c.repRate());
    blockSignals(false);

    ui->pulsePlot->newConfig(c);
}

void PulseConfigWidget::changeDelay(int i, double d)
{
    if(d_widgetList.at(i).endActive)
        emit changeSetting(i,QtFTM::PulseWidth,qAbs(d_widgetList.at(i).endBox->value() - d));
    emit changeSetting(i,QtFTM::PulseDelay,d);

}

void PulseConfigWidget::changeWidth(int i, double w)
{
    double d = d_widgetList.at(i).delayBox->value();
    if(!d_widgetList.at(i).endActive)
    {
        d_widgetList.at(i).endBox->blockSignals(true);
        d_widgetList.at(i).endBox->setValue(d+w);
        d_widgetList.at(i).endBox->blockSignals(false);
    }
    emit changeSetting(i,QtFTM::PulseWidth,w);
}

void PulseConfigWidget::changeEnd(int i, double e)
{
    if(d_widgetList.at(i).endActive)
    {
        double d = d_widgetList.at(i).delayBox->value();
        double w = qAbs(e-d);
        d_widgetList.at(i).widthBox->blockSignals(true);
        d_widgetList.at(i).widthBox->setValue(w);
        d_widgetList.at(i).widthBox->blockSignals(false);
        emit changeSetting(i,QtFTM::PulseWidth,w);
    }

}

void PulseConfigWidget::newRepRate(double r)
{
    ui->repRateBox->blockSignals(true);
    ui->repRateBox->setValue(r);
    ui->repRateBox->blockSignals(false);
    ui->pulsePlot->newRepRate(r);
}

void PulseConfigWidget::newProtDelay(int d)
{
	ui->protDelayBox->blockSignals(true);
	ui->protDelayBox->setValue(d);
	ui->protDelayBox->blockSignals(false);
	ui->pulsePlot->newProtDelay(d);
}

void PulseConfigWidget::newScopeDelay(int d)
{
	ui->scopeDelayBox->blockSignals(true);
	ui->scopeDelayBox->setValue(d);
	ui->scopeDelayBox->blockSignals(false);
	ui->pulsePlot->newScopeDelay(d);
}
