#include "amdorsetuppage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableView>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDoubleSpinBox>

#include "singlescanwidget.h"
#include "batchwidget.h"
#include "batchwizard.h"
#include "amdorbatch.h"
#include "amdordronlymodel.h"

AmdorSetupPage::AmdorSetupPage(SingleScanWidget *ssw, QWidget *parent) : QWizardPage(parent)
{
    setTitle("AMDOR setup");
    setSubTitle(QString("Enter parameters for all FTM+DR frequencies and any additional frequencies for DR only. Tuning and DR pulses will be set automatically."));

    QHBoxLayout *hl = new QHBoxLayout(this);

    p_bw = new BatchWidget(ssw,QtFTM::Amdor,this);
    connect(p_bw,&BatchWidget::scansChanged,this,&QWizardPage::completeChanged);

    hl->addWidget(p_bw,3);

    p_table = new QTableView(this);
    p_table->setAlternatingRowColors(true);
    p_model = new AmdorDrOnlyModel(this);

    connect(p_bw,&BatchWidget::amdorDrOnly,p_model,&AmdorDrOnlyModel::addEntry);
    connect(this,&AmdorSetupPage::drOnly,p_bw,&BatchWidget::setDrOnlyList);
    connect(p_bw,&BatchWidget::requestDrOnlyList,[=](){ emit drOnly(p_model->drList()); });
    connect(p_table->selectionModel(),&QItemSelectionModel::selectionChanged,this,&AmdorSetupPage::configureButtons);
    connect(p_model,&AmdorDrOnlyModel::modelChanged,this,&AmdorSetupPage::configureButtons);

    p_table->setModel(p_model);

    p_addButton = new QPushButton(QString("Add..."),this);
    connect(p_addButton,&QPushButton::clicked,[=](){ newEntryPopup(); });

    p_insertButton = new QPushButton(QString("Insert..."),this);
    connect(p_insertButton,&QPushButton::clicked,this,&AmdorSetupPage::insertCallback);

    p_removeButton = new QPushButton(QString("Remove"),this);
    connect(p_removeButton,&QPushButton::clicked,this,&AmdorSetupPage::removeCallback);

    p_insertButton->setEnabled(false);
    p_removeButton->setEnabled(false);

    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->addWidget(p_addButton,0,Qt::AlignCenter);
    hbl->addWidget(p_insertButton,0,Qt::AlignCenter);
    hbl->addWidget(p_removeButton,0,Qt::AlignCenter);

    QVBoxLayout *vl = new QVBoxLayout();
    vl->addWidget(p_table,1);
    vl->addLayout(hl,0);

    hl->addLayout(vl,2);

    setLayout(hl);
}



void AmdorSetupPage::initializePage()
{
    p_bw->updateLabel();
}

bool AmdorSetupPage::validatePage()
{
    BatchWizard *wiz = qobject_cast<BatchWizard*>(wizard());
    QList<QPair<Scan,bool> > scanList = p_bw->getList();

    if(field(QString("amdorCal")).toBool())
        scanList.prepend(qMakePair(wiz->calTemplate(),true));

    //get DR only scans
    QList<QPair<double,double>> drOnlyList = p_model->drList();

    double t = field(QString("amdorThreshold")).toDouble();
    double win = field(QString("amdorWindow")).toDouble();
    double exc = field(QString("amdorExclude")).toDouble();
    int ch = field(QString("amdorChildren")).toInt();

    AmdorBatch *b = new AmdorBatch(scanList,drOnlyList,t,win,exc,ch,wiz->fitter());
    b->setSleepWhenComplete(p_bw->sleep());

    emit batchManager(b);

    return true;

}

bool AmdorSetupPage::isComplete() const
{
    return !p_bw->isEmpty();
}

int AmdorSetupPage::nextId() const
{
    return -1;
}

void AmdorSetupPage::insertCallback()
{
    const QModelIndexList l = p_table->selectionModel()->selectedIndexes();

    int firstRow = l.first().row();
    for(int i=1; i<l.size(); i++)
        firstRow = qMin(l.at(i).row(),firstRow);

    newEntryPopup(firstRow);
}

void AmdorSetupPage::removeCallback()
{
    const QModelIndexList l = p_table->selectionModel()->selectedIndexes();
    QList<int> rows;

    for(int i=0; i<l.size(); i++)
    {
        if(!rows.contains(l.at(i).row()))
            rows.append(l.at(i).row());
    }

    p_model->removeEntries(rows);
}

void AmdorSetupPage::newEntryPopup(int pos)
{
    QDialog d(this);
    d.setWindowTitle(QString("New DR Only Entry"));

    QFormLayout *fl = new QFormLayout();
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("drSynth"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());

    QDoubleSpinBox *fBox = new QDoubleSpinBox(&d);
    fBox->setDecimals(3);
    fBox->setSuffix(QString(" MHz"));
    fBox->setRange(s.value(QString("min"),1000.0).toDouble(),s.value(QString("max"),1000000.0).toDouble());
    fBox->setValue(fBox->minimum());

    QDoubleSpinBox *pBox = new QDoubleSpinBox(&d);
    pBox->setDecimals(1);
    pBox->setSuffix(QString(" dBm"));
    pBox->setRange(s.value(QString("minPower"),-70.0).toDouble(),s.value(QString("maxPower"),17.0).toDouble());
    pBox->setValue(pBox->maximum());

    fl->addRow(QString("Frequency"),fBox);
    fl->addRow(QString("Power"),pBox);

    s.endGroup();
    s.endGroup();

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d);
    connect(bb->button(QDialogButtonBox::Ok),&QPushButton::clicked,&d,&QDialog::accept);
    connect(bb->button(QDialogButtonBox::Cancel),&QPushButton::clicked,&d,&QDialog::reject);

    QVBoxLayout *vl = new QVBoxLayout;
    vl->addLayout(fl,1);
    vl->addWidget(bb,0);

    d.setLayout(vl);

    if(d.exec() == QDialog::Accepted)
        p_model->insertEntry(fBox->value(),pBox->value(),pos);
}

void AmdorSetupPage::configureButtons()
{
    QModelIndexList l = p_table->selectionModel()->selectedIndexes();

    p_insertButton->setEnabled(!l.isEmpty());
    p_removeButton->setEnabled(!l.isEmpty());
}
