#include "startpage.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

#include "batchwizard.h"
StartPage::StartPage(double pressure, bool pEnabled, QList<QPair<double,bool>> flows, QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Choose a batch type"));
	setSubTitle(QString("Select the type of batch operation you would like."));

	QVBoxLayout *vl = new QVBoxLayout(this);

	surveyButton = new QRadioButton(QString("Survey"),this);
	surveyButton->setToolTip(QString("A series of scans yielding continuous frequency coverage"));

	drButton = new QRadioButton(QString("DR Scan"),this);
	drButton->setToolTip(QString("Scans the double resonance synthesizer while sitting at a single FT frequency"));

	batchButton = new QRadioButton(QString("Batch"),this);
	batchButton->setToolTip(QString("An arbitrary sequence of scans"));

    categorizeButton = new QRadioButton(QString("Categorize"),this);
    categorizeButton->setToolTip(QString("Perform a series of tests on a list of lines."));

	drCorrButton = new QRadioButton(QString("DR Correlation"),this);
	drCorrButton->setToolTip(QString("A list of FT scans that will by systematically cross-correlated by double resonance."));

    amdorButton = new QRadioButton(QString("AMDOR"),this);
    amdorButton->setToolTip(QString("Automated Microwave Double Resonance: systematic DR with optimizations to discover linked networks of lines quickly."));

	surveyButton->setChecked(true);

	vl->addWidget(surveyButton);
	vl->addWidget(drButton);
	vl->addWidget(batchButton);
    vl->addWidget(categorizeButton);
	vl->addWidget(drCorrButton);
    vl->addWidget(amdorButton);

	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	s.beginGroup(QString("ftmSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	double ftMin = s.value(QString("min"),5000.0).toDouble();
	double ftMax = s.value(QString("max"),26000.0).toDouble();
	s.endGroup();
	s.endGroup();

	s.beginGroup(QString("drSynth"));
	s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
	double drMin = s.value(QString("min"),1000.0).toDouble();
	double drMax = s.value(QString("max"),26000.0).toDouble();
	s.endGroup();
	s.endGroup();

	double min = qMax(ftMin,drMin);
	double max = qMin(ftMax,drMax);

	if(max < min)
	{
		drCorrButton->setEnabled(false);
        amdorButton->setEnabled(false);
        QLabel *lbl = new QLabel(QString("DR correlation and AMDOR are disabled because the DR synthesizer cannot access the range of the FT. Reconfigure the DR synthesizer to match the FT range and try again."));
		lbl->setWordWrap(true);
		vl->addWidget(lbl);
	}

    QGroupBox *limitsBox = new QGroupBox(QString("Limits"));
    QGridLayout *gl = new QGridLayout;
    gl->addWidget(new QLabel(QString("Setting")),0,0,1,1,Qt::AlignCenter);
    gl->addWidget(new QLabel(QString("Low Limit")),0,1,1,1,Qt::AlignCenter);
    gl->addWidget(new QLabel(QString("Current Value")),0,2,1,1,Qt::AlignCenter);
    gl->addWidget(new QLabel(QString("Upper Limit")),0,3,1,1,Qt::AlignCenter);
    gl->addWidget(new QLabel(QString("Enabled?")),0,4,1,1,Qt::AlignCenter);

    p_pressureMinBox = new QDoubleSpinBox(this);
    p_pressureMinBox->setRange(-100.0,100.0);
    p_pressureMinBox->setDecimals(3);
    p_pressureMinBox->setValue(0.9*pressure);
    p_pressureMinBox->setSuffix(QString(" kTorr"));
    p_pressureMinBox->setSingleStep(0.1*pressure);
    p_pressureMinBox->setEnabled(pEnabled);

    p_pressureMaxBox = new QDoubleSpinBox(this);
    p_pressureMaxBox->setRange(-100.0,100.0);
    p_pressureMaxBox->setDecimals(3);
    p_pressureMaxBox->setValue(1.1*pressure);
    p_pressureMaxBox->setSuffix(QString(" kTorr"));
    p_pressureMaxBox->setSingleStep(qAbs(0.1*pressure));
    p_pressureMaxBox->setEnabled(pEnabled);

    QDoubleSpinBox *pressureValBox = new QDoubleSpinBox(this);
    pressureValBox->setRange(-100.0,100.0);
    pressureValBox->setDecimals(3);
    pressureValBox->setValue(pressure);
    pressureValBox->setSuffix(QString(" kTorr"));
    pressureValBox->setReadOnly(true);
    pressureValBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

    p_pressureEnabledBox = new QCheckBox(this);
    p_pressureEnabledBox->setChecked(pEnabled);
    p_pressureEnabledBox->setEnabled(pEnabled);
    connect(p_pressureEnabledBox,&QCheckBox::toggled,p_pressureMinBox,&QDoubleSpinBox::setEnabled);
    connect(p_pressureEnabledBox,&QCheckBox::toggled,p_pressureMaxBox,&QDoubleSpinBox::setEnabled);

    gl->addWidget(new QLabel(QString("Pressure")),1,0,1,1,Qt::AlignCenter);
    gl->addWidget(p_pressureMinBox,1,1);
    gl->addWidget(pressureValBox,1,2);
    gl->addWidget(p_pressureMaxBox,1,3);
    gl->addWidget(p_pressureEnabledBox);

    for(int i=0;i<flows.size();i++)
    {
        QDoubleSpinBox *minBox = new QDoubleSpinBox(this);
        minBox->setRange(-10000.0,10000.0);
        minBox->setDecimals(3);
        minBox->setValue(0.5*flows.at(i).first);
        minBox->setSuffix(QString(" sccm"));
        minBox->setSingleStep(qAbs(0.1*flows.at(i).first));
        minBox->setEnabled(flows.at(i).second);

        QDoubleSpinBox *maxBox = new QDoubleSpinBox(this);
        maxBox->setRange(-10000.0,10000.0);
        maxBox->setDecimals(3);
        maxBox->setValue(1.5*flows.at(i).first);
        maxBox->setSuffix(QString(" sccm"));
        maxBox->setSingleStep(qAbs(0.1*flows.at(i).first));
        maxBox->setEnabled(flows.at(i).second);

        QDoubleSpinBox *valBox = new QDoubleSpinBox(this);
        valBox->setRange(-10000.0,10000.0);
        valBox->setDecimals(3);
        valBox->setValue(flows.at(i).first);
        valBox->setSuffix(QString(" sccm"));
        valBox->setReadOnly(true);
        valBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

        QCheckBox *enBox = new QCheckBox(this);
        enBox->setChecked(flows.at(i).second);
        enBox->setEnabled(flows.at(i).second);
        connect(enBox,&QCheckBox::toggled,minBox,&QDoubleSpinBox::setEnabled);
        connect(enBox,&QCheckBox::toggled,maxBox,&QDoubleSpinBox::setEnabled);

        FlowLimitWidgets flw;
        flw.minBox = minBox;
        flw.valBox = valBox;
        flw.enabledBox = enBox;
        flw.maxBox = maxBox;

        d_flowLimits.append(flw);

        gl->addWidget(new QLabel(QString("Gas %1").arg(i+1)),2+i,0,1,1,Qt::AlignCenter);
        gl->addWidget(minBox,2+i,1);
        gl->addWidget(valBox,2+i,2);
        gl->addWidget(maxBox,2+i,3);
        gl->addWidget(enBox,2+i,4);
    }

    gl->setColumnStretch(0,0);
    gl->setColumnStretch(1,1);
    gl->setColumnStretch(2,1);
    gl->setColumnStretch(3,1);
    gl->setColumnStretch(4,0);
    limitsBox->setLayout(gl);

    vl->addWidget(limitsBox);

	setLayout(vl);

}

int StartPage::nextId() const
{
	if(surveyButton->isChecked())
		return BatchWizard::Page_SurveySetup;

	if(drButton->isChecked())
		return BatchWizard::Page_DrSetup;

	if(batchButton->isChecked())
		return BatchWizard::Page_BatchProcessing;

    if(categorizeButton->isChecked())
        return BatchWizard::Page_BatchProcessing;

	if(drCorrButton->isChecked())
		return BatchWizard::Page_BatchProcessing;

    if(amdorButton->isChecked())
        return BatchWizard::Page_AmdorConfig;

	return -1;
}

bool StartPage::validatePage()
{
	if(surveyButton->isChecked())
		emit typeSelected(QtFTM::Survey);
	else if(drButton->isChecked())
		emit typeSelected(QtFTM::DrScan);
	else if(batchButton->isChecked())
		emit typeSelected(QtFTM::Batch);
    else if(categorizeButton->isChecked())
        emit typeSelected(QtFTM::Categorize);
	else if(drCorrButton->isChecked())
		emit typeSelected(QtFTM::DrCorrelation);

    return true;
}

void StartPage::applyLimits(BatchManager *bm)
{
    if(p_pressureEnabledBox->isChecked())
        bm->setPressureLimits(p_pressureMinBox->value(),p_pressureMaxBox->value());

    for(int i=0; i<d_flowLimits.size(); i++)
        bm->addFlowLimit(d_flowLimits.at(i).enabledBox->isChecked(),d_flowLimits.at(i).minBox->value(),d_flowLimits.at(i).maxBox->value());
}
