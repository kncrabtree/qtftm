#ifndef SINGLESCANDIALOG_H
#define SINGLESCANDIALOG_H

#include <QDialog>
#include "singlescanwidget.h"
#include "autofitwidget.h"

namespace Ui {
class SingleScanDialog;
}

class SingleScanDialog : public QDialog
{
	Q_OBJECT

private:
	Ui::SingleScanDialog *ui;

public:
	explicit SingleScanDialog(QWidget *parent = nullptr);
	~SingleScanDialog();

	SingleScanWidget *ssWidget();
	void insertAutoFitWidget(AutoFitWidget *w);

	

};

#endif // SINGLESCANDIALOG_H
