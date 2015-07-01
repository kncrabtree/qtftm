#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

SettingsDialog::SettingsDialog(SettingsWidget *w, QWidget *parent) :
	QDialog(parent)
{
	QVBoxLayout *vbl = new QVBoxLayout;

	vbl->addWidget(w);

	QDialogButtonBox *bb = new QDialogButtonBox();
	bb->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
					   QDialogButtonBox::RestoreDefaults);

	vbl->addWidget(bb);
	setLayout(vbl);

	bb->button(QDialogButtonBox::RestoreDefaults)->setEnabled(false);
	bb->button(QDialogButtonBox::Apply)->setEnabled(false);

	connect(w,&SettingsWidget::somethingChanged,bb->button(QDialogButtonBox::RestoreDefaults),&QWidget::setEnabled);
	connect(w,&SettingsWidget::somethingChanged,bb->button(QDialogButtonBox::Apply),&QWidget::setEnabled);
	connect(bb,&QDialogButtonBox::accepted,w,&SettingsWidget::saveSettings);
	connect(bb->button(QDialogButtonBox::RestoreDefaults),&QAbstractButton::clicked,w,&SettingsWidget::loadSettings);
	connect(bb->button(QDialogButtonBox::RestoreDefaults),&QAbstractButton::clicked,
		   bb->button(QDialogButtonBox::RestoreDefaults),&QWidget::setEnabled);
	connect(bb->button(QDialogButtonBox::RestoreDefaults),&QAbstractButton::clicked,
		   bb->button(QDialogButtonBox::Apply),&QWidget::setEnabled);
	connect(bb->button(QDialogButtonBox::Apply),&QAbstractButton::clicked,w,&SettingsWidget::saveSettings);
	connect(bb->button(QDialogButtonBox::Apply),&QAbstractButton::clicked,
		   bb->button(QDialogButtonBox::Apply),&QWidget::setEnabled);
	connect(bb->button(QDialogButtonBox::Apply),&QAbstractButton::clicked,
		   bb->button(QDialogButtonBox::RestoreDefaults),&QWidget::setEnabled);
	connect(bb,&QDialogButtonBox::accepted,this,&QDialog::accept);
	connect(bb,&QDialogButtonBox::rejected,this,&QDialog::reject);

	setWindowTitle(w->title());

}

SettingsDialog::~SettingsDialog()
{
}
