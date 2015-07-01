#include "drsummarypage.h"
#include <QSettings>

DrSummaryPage::DrSummaryPage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("DR Summary"));
	setSubTitle(QString("These are the settings you have chosen. If any changes need to be made, use the back button."));

	QVBoxLayout *vl = new QVBoxLayout(this);
	label = new QLabel(this);
	label->setText(QString(""));

	vl->addWidget(label,0,Qt::AlignCenter);

    sleepCheckBox = new QCheckBox(QString("Sleep when batch is complete"),this);
    sleepCheckBox->setToolTip(QString("If checked, the instrument will be put into sleep mode when the acquisition is complete.\nThis will stop pulses from being generated, and will turn off the gas flow controllers."));
    sleepCheckBox->setChecked(false);
    vl->addWidget(sleepCheckBox,0,Qt::AlignCenter);

	setLayout(vl);
}

int DrSummaryPage::nextId() const
{
	return -1;
}

void DrSummaryPage::initializePage()
{
	//this is all pretty straightforward
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

	int drNum = s.value("drNum",1).toInt();
	int firstScanNum = s.value("scanNum",0).toInt()+1;
	bool hasCal = field(QString("drCal")).toBool();
	double start = field(QString("drStart")).toDouble();
	double stop = field(QString("drStop")).toDouble();
	int step = field(QString("drStep")).toInt();

    int numScans = field(QString("drScansBetweenTuningVoltageReadings")).toInt();

	double del = field(QString("drDelay")).toDouble();
	double hpf = field(QString("drHpf")).toDouble();
	double exp = field(QString("drExp")).toDouble();
    bool rDC = field(QString("drRemoveDc")).toBool();
	bool padFid = field(QString("drPadFid")).toBool();
	bool autoFit = field(QString("drAutoFit")).toBool();

	//need to get the integration ranges from the batch wizard
	BatchWizard *bw = qobject_cast<BatchWizard*>(wizard());
	QList<QPair<double,double> > ranges = bw->ranges();

	QString text;
	text.append(QString("<table>"));
	text.append(QString("<tr><td>DR scan number </td><td align='right'> %1 </td><td> </td></tr>").arg(drNum));
	text.append(QString("<tr><td>First scan number </td><td align='right'> %1 </td><td> </td></tr>").arg(firstScanNum));
	text.append(QString("<tr><td>Start freq </td><td align='right'> %1 </td><td> MHz</td></tr>").arg(start,0,'f',3));
	text.append(QString("<tr><td>Stop freq </td><td align='right'> %1 </td><td> MHz</td></tr>").arg(stop,0,'f',3));
	text.append(QString("<tr><td>Step size </td><td align='right'> %1 </td><td> kHz</td></tr>").arg(step));
    text.append(QString("<tr><td>Scans between Tuning Voltage Readings (0=every scan)</td><td> align='right'> %1 </td><td> scans</td><tr>").arg((numScans)));
	if(hasCal)
		text.append(QString("<tr><td>Has calibration? </td><td align='right'> Yes </td><td></td></tr>"));
	else
		text.append(QString("<tr><td>Has calibration? </td><td align='right'> No </td><td></td></tr>"));
	text.append(QString("<tr><td>FID delay </td><td align='right'> %1 </td><td> &mu;s</td></tr>").arg(del,0,'f',1));
	text.append(QString("<tr><td>FID high pass </td><td align='right'> %1 </td><td> kHz</td></tr>").arg(hpf,0,'f',0));
	text.append(QString("<tr><td>FID exp decay </td><td align='right'> %1 </td><td> &mu;s</td></tr>").arg(exp,0,'f',1));
	if(rDC)
		text.append(QString("<tr><td>Remove FID DC? </td><td align='right'> Yes </td><td></td></tr>"));
	else
		text.append(QString("<tr><td>Remove FID DC? </td><td align='right'> No </td><td></td></tr>"));
	if(padFid)
		text.append(QString("<tr><td>Zero-pad FID? </td><td align='right'> Yes </td><td></td></tr>"));
	else
		text.append(QString("<tr><td>Zero-pad FID? </td><td align='right'> No </td><td></td></tr>"));
	if(autoFit)
		text.append(QString("<tr><td>AutoFit Enabled? </td><td align='right'> Yes </td><td></td></tr>"));
	else
		text.append(QString("<tr><td>AutoFit Enabled? </td><td align='right'> No </td><td></td></tr>"));
	text.append(QString("<tr><td colspan=3> </td></tr><tr><th colspan=3>Integration ranges</th></tr>"));
	for(int i=0; i<ranges.size(); i++)
		text.append(QString("<tr><td>Range %1 </td><td colspan=2> %2 - %3 MHz </td></tr>").arg(i)
				.arg(ranges.at(i).first,0,'f',4).arg(ranges.at(i).second,0,'f',4));
	text.append(QString("</table>"));

	label->setText(text);
}

bool DrSummaryPage::validatePage()
{
	BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());

	BatchDR *bdr;

	//get the ftm settings
	Scan s = wiz->drScan();

	//set number of shots
	s.setTargetShots(field(QString("drShots")).toInt());

	//get settings needed for BatchDr object
	double start = field(QString("drStart")).toDouble();
	double stop = field(QString("drStop")).toDouble();
	double step = field(QString("drStep")).toDouble()/1000.0;
	bool hasCal = field(QString("drCal")).toBool();
    int numScansBetween = field(QString("drScansBetweenTuningVoltageReadings")).toInt();
	QList<QPair<double,double> > r = wiz->ranges();

	AbstractFitter *fitter = wiz->fitter();

	double del = field(QString("drDelay")).toDouble();
	double hpf = field(QString("drHpf")).toDouble();
	double exp = field(QString("drExp")).toDouble();
	bool rDC = field(QString("drRemoveDc")).toBool();
	bool padFid = field(QString("drPadFid")).toBool();

	//set filtering settings
	fitter->setDelay(del);
	fitter->setHpf(hpf);
	fitter->setExp(exp);
	fitter->setRemoveDC(rDC);
	fitter->setAutoPad(padFid);

    emit sleepWhenComplete(sleepCheckBox->isChecked());

	//make BatchDR
    bdr = new BatchDR(s,start,stop,step,numScansBetween,r,hasCal,fitter);
	emit batchDr(bdr);

	return true;
}
