#include "surveysummarypage.h"
#include <QSettings>
#include <QApplication>
#include <QVBoxLayout>

SurveySummaryPage::SurveySummaryPage(QWidget *parent) :
     QWizardPage(parent)
{
	setTitle(QString("Survey Summary"));
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

int SurveySummaryPage::nextId() const
{
	return -1;
}

void SurveySummaryPage::initializePage()
{
	QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());

	//get settings and show them
	int surveyNum = s.value("surveyNum",1).toInt();
	int firstScanNum = s.value("scanNum",0).toInt()+1;
	bool hasCal = field(QString("surveyCal")).toBool();
	double start = field(QString("surveyStart")).toDouble();
	double stop = field(QString("surveyStop")).toDouble();
	int step = field(QString("surveyStep")).toInt();

    BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());

    double del = wiz->fitter()->delay();
    double hpf = wiz->fitter()->hpf();
    double exp = wiz->fitter()->exp();
    bool rDC = wiz->fitter()->removeDC();
    bool padFid = wiz->fitter()->autoPad();

	QString text;
	text.append(QString("<table>"));
	text.append(QString("<tr><td>Survey number </td><td align='right'> %1 </td><td> </td></tr>").arg(surveyNum));
	text.append(QString("<tr><td>First scan number </td><td align='right'> %1 </td><td> </td></tr>").arg(firstScanNum));
	text.append(QString("<tr><td>Start freq </td><td align='right'> %1 </td><td> MHz</td></tr>").arg(start,0,'f',3));
	text.append(QString("<tr><td>Stop freq </td><td align='right'> %1 </td><td> MHz</td></tr>").arg(stop,0,'f',3));
	text.append(QString("<tr><td>Step size </td><td align='right'> %1 </td><td> kHz</td></tr>").arg(step));
	if(hasCal)
	{
		text.append(QString("<tr><td>Has calibration? </td><td align='right'> Yes </td><td></td></tr>"));
		int scansPerCal = field(QString("surveyScansPerCal")).toInt();
		text.append(QString("<tr><td>Scans per calibration </td><td align='right'> %1 </td><td></td></tr>").arg(scansPerCal));
	}
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
	text.append(QString("</table>"));

	label->setText(text);

}

bool SurveySummaryPage::validatePage()
{
	BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());

	BatchSurvey *bs;

	//get template from wizard
	Scan surveyTemplate = wiz->surveyScan();
	double stop = field(QString("surveyStop")).toDouble();
	double step = field(QString("surveyStep")).toDouble()/1000.0;
	bool hasCal = field(QString("surveyCal")).toBool();
	if(hasCal)
	{
		//get cal template from wizard
		Scan calTemplate = wiz->surveyCal();
		int scansPerCal = field(QString("surveyScansPerCal")).toInt();

		//create survey object with cal
        bs = new BatchSurvey(surveyTemplate,step,stop,true,calTemplate,scansPerCal,wiz->fitter());
	}
	else //create survey object without cal
        bs = new BatchSurvey(surveyTemplate,step,stop,false,Scan(),0,wiz->fitter());

    emit sleepWhenComplete(sleepCheckBox->isChecked());
	emit batchSurvey(bs);

	return true;
}
