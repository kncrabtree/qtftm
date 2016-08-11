#include "autofitwidget.h"
#include "ui_autofitwidget.h"

AutoFitWidget::AutoFitWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoFitWidget)
{
    ui->setupUi(this);

    connect(ui->autoFitEnabledCheckBox,&QAbstractButton::toggled,ui->autoFitSettingsBox,&QWidget::setEnabled);
    connect(ui->autoFitEnabledCheckBox,&QAbstractButton::toggled,this,&AutoFitWidget::autoFitEnabledChanged);

    d_gases << Analysis::bufferH2 << Analysis::bufferHe << Analysis::bufferN2 << Analysis::bufferO2
           << Analysis::bufferNe << Analysis::bufferAr << Analysis::bufferKr << Analysis::bufferXe;
}

AutoFitWidget::AutoFitWidget(QString bgName, double d, double h, double e, bool zp, bool winf, double t, QWidget *parent) :
     QWidget(parent),
     ui(new Ui::AutoFitWidget)
{
	ui->setupUi(this);

	connect(ui->autoFitEnabledCheckBox,&QAbstractButton::toggled,ui->autoFitSettingsBox,&QWidget::setEnabled);
	connect(ui->autoFitEnabledCheckBox,&QAbstractButton::toggled,this,&AutoFitWidget::autoFitEnabledChanged);

	ui->delayDoubleSpinBox->setValue(d);
	ui->highPassDoubleSpinBox->setValue(h);
	ui->expFilterDoubleSpinBox->setValue(e);
	ui->zeroPadFIDCheckBox->setChecked(zp);
    ui->applyBHWindowCheckBox->setChecked(winf);
	ui->temperatureDoubleSpinBox->setValue(t);

	d_gases << Analysis::bufferH2 << Analysis::bufferHe << Analysis::bufferN2 << Analysis::bufferO2
		   << Analysis::bufferNe << Analysis::bufferAr << Analysis::bufferKr << Analysis::bufferXe;

	//default buffer gas is Ne. Check to see if the bgname contains something different
	if(bgName.contains(QString("H2"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(0);
	if(bgName.contains(QString("He"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(1);
	if(bgName.contains(QString("N2"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(2);
	if(bgName.contains(QString("O2"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(3);
	if(bgName.contains(QString("Ne"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(4);
	if(bgName.contains(QString("Ar"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(5);
	if(bgName.contains(QString("Kr"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(6);
	if(bgName.contains(QString("Xe"),Qt::CaseSensitive))
		ui->bufferGasComboBox->setCurrentIndex(7);
}

AutoFitWidget::~AutoFitWidget()
{
	delete ui;
}

bool AutoFitWidget::autoFitEnabled() const
{
	return ui->autoFitEnabledCheckBox->isChecked();
}

int AutoFitWidget::sourceConfig() const
{
	return ui->sourceConfigComboBox->currentIndex();
}

FitResult::BufferGas AutoFitWidget::bufferGas() const
{
	return d_gases.at(ui->bufferGasComboBox->currentIndex());
}

double AutoFitWidget::temperature() const
{
	return ui->temperatureDoubleSpinBox->value();
}

double AutoFitWidget::delay() const
{
	return ui->delayDoubleSpinBox->value();
}

double AutoFitWidget::hpf() const
{
	return ui->highPassDoubleSpinBox->value();
}

double AutoFitWidget::exp() const
{
	return ui->expFilterDoubleSpinBox->value();
}

bool AutoFitWidget::removeDC() const
{
	return ui->removeDCCheckBox->isChecked();
}

bool AutoFitWidget::zeroPad() const
{
    return ui->zeroPadFIDCheckBox->isChecked();
}

bool AutoFitWidget::isUseWindow() const
{
    return ui->applyBHWindowCheckBox->isChecked();
}

void AutoFitWidget::setNoDisable()
{
	ui->autoFitEnabledCheckBox->setChecked(true);
	ui->autoFitEnabledCheckBox->setEnabled(false);
}

void AutoFitWidget::disableProcessingOptions(bool disable)
{
	ui->processingSettingsBox->setDisabled(disable);
    ui->processingSettingsBox->setVisible(!disable);
}

void AutoFitWidget::setFromFitter(AbstractFitter *af)
{
    if(af != nullptr)
    {
        if(af->type() == FitResult::NoFitting)
            ui->autoFitEnabledCheckBox->setChecked(false);
        else
            ui->autoFitEnabledCheckBox->setChecked(true);

        if(af->type() == FitResult::DopplerPair)
        {
            //update indices if non-coaxial configurations are added
            ui->sourceConfigComboBox->setCurrentIndex(0);
        }
        ui->bufferGasComboBox->setCurrentIndex(gasIndex(af->bufferGas()));
        ui->temperatureDoubleSpinBox->setValue(af->temperature());
        ui->delayDoubleSpinBox->setValue(af->delay());
        ui->highPassDoubleSpinBox->setValue(af->hpf());
        ui->expFilterDoubleSpinBox->setValue(af->exp());
        ui->removeDCCheckBox->setChecked(af->removeDC());
        ui->zeroPadFIDCheckBox->setChecked(af->autoPad());
        ui->applyBHWindowCheckBox->setChecked(af->isUseWindow());
    }
}

AbstractFitter *AutoFitWidget::toFitter()
{
	AbstractFitter *af;
	if(!autoFitEnabled())
	{
		af = new NoFitter();
	}
	else
	{
        af = new DopplerPairFitter();
		af->setBufferGas(bufferGas());
		af->setTemperature(temperature());
	}

	af->setDelay(delay());
	af->setHpf(hpf());
	af->setExp(exp());
	af->setRemoveDC(removeDC());
	af->setAutoPad(zeroPad());
    af->setUseWindow(isUseWindow());

    return af;
}

void AutoFitWidget::setAutofitEnabled(bool enabled)
{
    ui->autoFitEnabledCheckBox->setChecked(enabled);
}

int AutoFitWidget::gasIndex(const FitResult::BufferGas &gas)
{
    if(gas.name.contains(QString("H2"),Qt::CaseSensitive))
        return 0;
    if(gas.name.contains(QString("He"),Qt::CaseSensitive))
        return 1;
    if(gas.name.contains(QString("N2"),Qt::CaseSensitive))
        return 2;
    if(gas.name.contains(QString("O2"),Qt::CaseSensitive))
        return 3;
    if(gas.name.contains(QString("Ne"),Qt::CaseSensitive))
        return 4;
    if(gas.name.contains(QString("Ar"),Qt::CaseSensitive))
        return 5;
    if(gas.name.contains(QString("Kr"),Qt::CaseSensitive))
        return 6;
    if(gas.name.contains(QString("Xe"),Qt::CaseSensitive))
        return 7;

    return 0;
}
