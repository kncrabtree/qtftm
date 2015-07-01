#include "ioboardconfigdialog.h"
#include "ui_ioboardconfigdialog.h"
#include <QSettings>
#include <QMessageBox>

IOBoardConfigDialog::IOBoardConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IOBoardConfigDialog), d_key(QString("ioboard"))
{
    ui->setupUi(this);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);

    ui->serialNoSpinBox->setValue(s.value(QString("serialNo"),3).toInt());
    ui->cwLineSpinBox->setValue(s.value(QString("cwLine"),16).toInt()-16);
    ui->highBandLineSpinBox->setValue(s.value(QString("highBandLine"),17).toInt()-16);
    ui->magnetLineSpinBox->setValue(s.value(QString("magnetLine"),18).toInt()-16);
    ui->counterTimerOffsetSpinBox->setValue(s.value(QString("counterOffset"),4).toInt());

    s.endGroup();

    auto intVc = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);

    connect(ui->serialNoSpinBox,intVc,this,&IOBoardConfigDialog::dirtySerialNumber);
    connect(ui->cwLineSpinBox,intVc,this,&IOBoardConfigDialog::validate);
    connect(ui->highBandLineSpinBox,intVc,this,&IOBoardConfigDialog::validate);
    connect(ui->magnetLineSpinBox,intVc,this,&IOBoardConfigDialog::validate);
    connect(ui->testConnectionButton,&QAbstractButton::clicked,this,&IOBoardConfigDialog::testConnectionCallback);

    validate();

}

IOBoardConfigDialog::~IOBoardConfigDialog()
{
    delete ui;
}

void IOBoardConfigDialog::validate()
{
	QSpinBox *changedBox, *other1, *other2;

	changedBox = qobject_cast<QSpinBox*>(sender());
	if(!changedBox)
		return;

	if(changedBox == ui->cwLineSpinBox)
	{
		other1 = ui->highBandLineSpinBox;
		other2 = ui->magnetLineSpinBox;
	}
	else if(changedBox == ui->highBandLineSpinBox)
	{
		other1 = ui->cwLineSpinBox;
		other2 = ui->magnetLineSpinBox;
	}
	else if(changedBox == ui->magnetLineSpinBox)
	{
		other1 = ui->cwLineSpinBox;
		other2 = ui->highBandLineSpinBox;
	}
	else
		return;

	//NOTE: ranges of spinboxes must be the same
    if(changedBox->value() == other1->value() || changedBox->value() == other2->value())
    {
	    bool goodFound = false;
	    int bestValue;
	    int badValue = changedBox->value();
	    for(int i=changedBox->minimum(); i <= changedBox->maximum(); i++)
	    {
		    if(i != other1->value() && i != other2->value())
		    {
			    if(!goodFound)
			    {
				    goodFound = true;
				    bestValue = i;
			    }
			    else if(abs(i-badValue) < abs(badValue-bestValue))
				    bestValue = i;
		    }
	    }

	    if(!goodFound)
		    return;

	    changedBox->setValue(bestValue);
    }
}

void IOBoardConfigDialog::dirtySerialNumber()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void IOBoardConfigDialog::testConnectionCallback()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.setValue(QString("serialNo"),ui->serialNoSpinBox->value());
    s.endGroup();
    s.sync();

    setCursor(Qt::BusyCursor);
    setEnabled(false);
    emit testConnection(QString(""),d_key);
}

void IOBoardConfigDialog::testComplete(QString device, bool success, QString msg)
{
    //configure ui
    setEnabled(true);
    setCursor(QCursor());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    if(success)
        QMessageBox::information(this,QString("Connection Successful"),
                            QString("%1 connected successfully!").arg(device),QMessageBox::Ok);
    else
        QMessageBox::critical(this,QString("Connection failed"),
                              QString("%1 connection failed!\n%2").arg(device).arg(msg),QMessageBox::Ok);
}

void IOBoardConfigDialog::accept()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.setValue(QString("cwLine"),ui->cwLineSpinBox->value()+16);
    s.setValue(QString("highBandLine"),ui->highBandLineSpinBox->value()+16);
    s.setValue(QString("magnetLine"),ui->magnetLineSpinBox->value()+16);
    s.setValue(QString("counterOffset"),ui->counterTimerOffsetSpinBox->value());
    s.endGroup();
    s.sync();

    QDialog::accept();
}
