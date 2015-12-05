#include "synthsettingswidget.h"
#include "ui_synthsettingswidget.h"
#include "QMessageBox"


SynthSettingsWidget::SynthSettingsWidget(QWidget *parent) :
	SettingsWidget(parent),
	ui(new Ui::SynthSettingsWidget), d_bandInfoChanged(false), d_lastBand(0)
{
	ui->setupUi(this);

	auto doubleVc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
	auto intVc = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);
	auto cmbVc = static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

	connect(ui->numBandsSpinBox,intVc,this,&SynthSettingsWidget::updateBandsComboBox);
	connect(ui->minDoubleSpinBox,doubleVc,this,&SynthSettingsWidget::bandInfoChanged);
	connect(ui->maxDoubleSpinBox,doubleVc,this,&SynthSettingsWidget::bandInfoChanged);
	connect(ui->multSpinBox,intVc,this,&SynthSettingsWidget::bandInfoChanged);
	connect(ui->switchComboBox,cmbVc,this,&SynthSettingsWidget::bandInfoChanged);

	connect(ui->bandComboBox,cmbVc,this,&SynthSettingsWidget::loadBandInfo);

	connect(ui->autoSwitchCheckBox,&QAbstractButton::clicked,this,&SettingsWidget::somethingChanged);
	connect(ui->numBandsSpinBox,intVc,this,&SettingsWidget::somethingChanged);
	connect(ui->maxDoubleSpinBox,doubleVc,this,&SettingsWidget::somethingChanged);
	connect(ui->minDoubleSpinBox,doubleVc,this,&SettingsWidget::somethingChanged);
	connect(ui->switchComboBox,cmbVc,this,&SettingsWidget::somethingChanged);
    connect(ui->switchComboBox,cmbVc,this,&SynthSettingsWidget::switchPointChanged);
	connect(ui->multSpinBox,intVc,this,&SettingsWidget::somethingChanged);
	connect(ui->bandComboBox,cmbVc,this,&SettingsWidget::somethingChanged);
}

SynthSettingsWidget::~SynthSettingsWidget()
{
	delete ui;
}

void SynthSettingsWidget::loadSettings()
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    d_subKey = s.value(QString("subKey"),QString("virtual")).toString();

    s.beginGroup(d_subKey);

	ui->numBandsSpinBox->setValue(s.value(QString("numBands"),1).toInt());
    ui->autoSwitchCheckBox->setChecked(s.value(QString("autoSwitch"),false).toBool());
    ui->bandComboBox->setCurrentIndex(s.value(QString("currentBand"),0).toInt());
	s.endGroup();
    s.endGroup();

    if(d_key == QString("drSynth"))
    {
        ui->autoSwitchCheckBox->setChecked(false);
        ui->autoSwitchCheckBox->setEnabled(false);
    }

	loadBandInfo();

}

void SynthSettingsWidget::saveSettings()
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);

	s.setValue(QString("numBands"),ui->numBandsSpinBox->value());
	s.setValue(QString("autoSwitch"),ui->autoSwitchCheckBox->isChecked());

	saveBandSettings(ui->bandComboBox->currentIndex());
    bool doBandChange = false;
	if(ui->autoSwitchCheckBox->isChecked())
	{
		int tot = s.beginReadArray(QString("bands"));
		s.setArrayIndex(0);
        double min = s.value(QString("min"),50.0).toDouble();
        double max = s.value(QString("max"),26000.0).toDouble();
		for(int i=1; i<tot; i++)
		{
			s.setArrayIndex(i);
            min = qMin(min,s.value(QString("min"),50.0).toDouble());
            max = qMax(max,s.value(QString("max"),26000.0).toDouble());
		}
		s.endArray();

		s.setValue(QString("min"),min);
		s.setValue(QString("max"),max);
	}
	else
	{
		s.setValue(QString("currentBand"),ui->bandComboBox->currentIndex());
		s.beginReadArray(QString("bands"));
		s.setArrayIndex(ui->bandComboBox->currentIndex());
		s.setValue(QString("min"),ui->minDoubleSpinBox->value());
		s.setValue(QString("max"),ui->maxDoubleSpinBox->value());
		s.endArray();

        doBandChange = true;

	}
	s.endGroup();
    s.endGroup();

	s.sync();

	d_bandInfoChanged = false;
    if(doBandChange)
        emit bandChanged();
}

void SynthSettingsWidget::updateBandsComboBox()
{
	if(ui->numBandsSpinBox->value() == ui->bandComboBox->count())
		return;

	if(ui->numBandsSpinBox->value() > ui->bandComboBox->count())
	{
		for(int i=ui->bandComboBox->count(); i<ui->numBandsSpinBox->value(); i++)
			ui->bandComboBox->addItem(QString::number(i));
	}
	else
	{
		for(int i=ui->bandComboBox->count(); i>ui->numBandsSpinBox->value(); i--)
			ui->bandComboBox->removeItem(i-1);
	}
}

void SynthSettingsWidget::loadBandInfo()
{

	if(d_bandInfoChanged)
	{
		int button = QMessageBox::question(this,QString("Save Changes?"),QString("Do you want to save the changes you made to Band %1?").arg(d_lastBand),QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save);

		if(button == QMessageBox::Save)
			saveBandSettings(d_lastBand);
	}

	bool ok = false;
	int band = ui->bandComboBox->itemText(ui->bandComboBox->currentIndex()).toInt(&ok);

	if(!ok || band<0)
	{
		ui->minDoubleSpinBox->setEnabled(false);
		ui->maxDoubleSpinBox->setEnabled(false);
		ui->multSpinBox->setEnabled(false);
		ui->switchComboBox->setEnabled(false);

		return;
	}

	d_lastBand = band;

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
	s.beginReadArray(QString("bands"));

	ui->minDoubleSpinBox->setEnabled(true);
	ui->maxDoubleSpinBox->setEnabled(true);
	ui->multSpinBox->setEnabled(true);
	ui->switchComboBox->setEnabled(ui->autoSwitchCheckBox->isChecked());

	//first, we need to make sure the boundaries on the spinboxes are set appropriately
	//get min value for previous band, and set that as minimum value of both spinboxes
	blockSignals(true);

	if(band == 0)
	{
        ui->minDoubleSpinBox->setMinimum(50.0);
        ui->maxDoubleSpinBox->setMinimum(50.0);
	}
	else
	{
		s.setArrayIndex(band-1);
        double m = s.value(QString("min"),50.0).toDouble();
		ui->minDoubleSpinBox->setMinimum(m);
		ui->maxDoubleSpinBox->setMinimum(m);
	}

	s.setArrayIndex(band);

    double m = s.value(QString("min"),50.0).toDouble();
	ui->minDoubleSpinBox->setValue(m);
	ui->maxDoubleSpinBox->setMinimum(m+1.0);
	ui->maxDoubleSpinBox->setValue(s.value(QString("max"),1e6).toDouble());

	ui->multSpinBox->setValue(s.value(QString("mult"),1).toInt());
	ui->switchComboBox->setCurrentIndex(s.value(QString("switch"),0).toInt());
    ui->offsetDoubleSpinBox->setValue(s.value(QString("offset"),0.0).toDouble());

	d_bandInfoChanged = false;

	s.endArray();
	s.endGroup();
    s.endGroup();

	blockSignals(false);

}

void SynthSettingsWidget::saveBandSettings(int band)
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);
    s.beginWriteArray(QString("bands"));

//	for(int i=0;i<ui->numBandsSpinBox->value();i++)
//	{
		s.setArrayIndex(band);
		s.setValue(QString("min"),ui->minDoubleSpinBox->value());
		s.setValue(QString("max"),ui->maxDoubleSpinBox->value());
		s.setValue(QString("mult"),ui->multSpinBox->value());
		s.setValue(QString("switch"),ui->switchComboBox->currentIndex());
        s.setValue(QString("offset"),ui->offsetDoubleSpinBox->value());
//	}
	s.endArray();
	s.endGroup();
    s.endGroup();
	s.sync();

}
