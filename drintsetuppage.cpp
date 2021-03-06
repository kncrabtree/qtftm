#include "drintsetuppage.h"
#include "batchwizard.h"
#include <QSettings>
#include <math.h>
#include <QDateTime>

DrIntSetupPage::DrIntSetupPage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("DR Integration Range Setup"));
	setSubTitle(QString("Set number of shots and integration ranges (use \"Mark Peak\" context menu item). Place the red and blue markers at the ends of the integration regions."));

	//make and configure an analysis widget
	aw = new AnalysisWidget(this);
    aw->configureForDr();

	//add a reset button
	resetButton = new QPushButton(QString("Reset"),this);

	//add a box for number of shots
	shotsSpinBox = new QSpinBox(this);
	shotsSpinBox->setMinimum(1);
	shotsSpinBox->setMaximum(1000000000);
	shotsSpinBox->setSingleStep(10);
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
	shotsSpinBox->setValue(s.value(QString("peakUpAvgs"),20).toInt());
	shotsSpinBox->setKeyboardTracking(false);
	shotsSpinBox->setToolTip(QString("Number of shots to acquire. The plot will show a rolling average of this number of shots"));
	connect(shotsSpinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),this,&DrIntSetupPage::updateLabel);

	shotsLabel = new QLabel(QString("Shots"),this);
    shotsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	aw->ui->topLayout->addWidget(shotsLabel);
	aw->ui->topLayout->addWidget(shotsSpinBox);
	aw->ui->topLayout->addWidget(resetButton);

	//label for time estimate
	timeLabel = new QLabel(this);
	aw->ui->leftVerticalLayout->addWidget(timeLabel,0,Qt::AlignCenter);

	QVBoxLayout *vl = new QVBoxLayout(this);
	vl->addWidget(aw);

	setLayout(vl);

	//register relevant fields for use later
	registerField(QString("drShots"),shotsSpinBox);
	registerField(QString("drDelay"),aw->ui->analysisPlot->getDelayBox());
	registerField(QString("drHpf"),aw->ui->analysisPlot->getHpfBox());
	registerField(QString("drExp"),aw->ui->analysisPlot->getExpBox());
	registerField(QString("drRemoveDc"),aw->ui->analysisPlot->getRemoveDcBox());
    registerField(QString("drPadFid"),aw->ui->analysisPlot->getPadFidBox());
    registerField(QString("drWindow"),aw->ui->analysisPlot->getUseWindowBox());

	connect(this,&DrIntSetupPage::newFid,aw->plot(),&FtPlot::newFid,Qt::UniqueConnection);
	connect(aw,&AnalysisWidget::linesChanged,this,&QWizardPage::completeChanged);



}

int DrIntSetupPage::nextId() const
{
	return BatchWizard::Page_DrSummary;
}

void DrIntSetupPage::initializePage()
{
	//make sure connections are only made once
	BatchWizard *bw = qobject_cast<BatchWizard*>(wizard());
	connect(resetButton,&QAbstractButton::clicked,bw,&BatchWizard::resetPeakUp,Qt::UniqueConnection);
	connect(shotsSpinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		   bw,&BatchWizard::changeNumPeakupFids,Qt::UniqueConnection);

	updateLabel();
}


bool DrIntSetupPage::validatePage()
{
	//send out the integration ranges for later use
	emit ranges(aw->ranges());
	return true;
}

void DrIntSetupPage::cleanupPage()
{
	//we can remove the fid connection, because the scan might be changed when we come back
	BatchWizard *bw = qobject_cast<BatchWizard*>(wizard());
	disconnect(bw,&BatchWizard::newFid,this,&DrIntSetupPage::newFid);
	aw->clearLines();
}

bool DrIntSetupPage::isComplete() const
{
	//must be at least one integration range
	return aw->numLines()>0;
}


void DrIntSetupPage::updateLabel()
{
	//calculate time based on rep rate
	double start = field(QString("drStart")).toDouble();
	double stop = field(QString("drStop")).toDouble();
	double step = field(QString("drStep")).toDouble()/1000.0;

	int scans = (int)ceil(fabs(stop-start)/step)+1.0;
	int calScans = 0;
	if(field(QString("drCal")).toBool())
		calScans = scans;

    double repRate = qobject_cast<BatchWizard*>(wizard())->scanTemplate().pulseConfiguration().repRate();
    int scansBetweenTune = field(QString("drScansBetweenTuningVoltageReadings")).toInt();
    int numTunes = (double)(scans)/(double)(scansBetweenTune+1);
    double delayTime = qobject_cast<BatchWizard*>(wizard())->scanTemplate().postTuneDelayShots()*(double)numTunes/repRate;

	//the first shot after starting a new scan is ignored, and it usually takes about one shot worth of time to measure
	//cavity voltage and intialize hardware.
    double time = (scans+calScans)*(shotsSpinBox->value()+2)/repRate + delayTime;
	int h = (int)floor(time/3600.0);
	int m = (int)floor(time)%3600/60;
	int s = (int)round(time)%60;

	QString text;
	text.append(QString("<table><tr><th colspan=2>Time estimate</th></tr>"));
	text.append(QString("<tr><td>Total time: </td><td> %1h %2m %3s </td></tr>").arg(h).arg(m).arg(s));
	QDateTime dt = QDateTime::currentDateTime().addSecs((int)round(time));
	text.append(QString("<tr><td>Completed at: </td><td> %1 </td></tr></table>").arg(dt.toString()));
	timeLabel->setText(text);
}
