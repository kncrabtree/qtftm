#include "amdorwidget.h"
#include "ui_amdorwidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSettings>
#include <QMessageBox>

AmdorWidget::AmdorWidget(AmdorData ad, int num, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AmdorWidget), d_editingPossible(false), d_livePlot(true), d_number(num)
{
    ui->setupUi(this);

    ui->refPlot->enableSelection(false);
    ui->drPlot->enableSelection(false);

    p_amdorModel = new AmdorDataModel(ad, this);
    p_proxyModel = new QSortFilterProxyModel(this);
    p_proxyModel->setSortRole(Qt::EditRole);
    p_proxyModel->setSourceModel(p_amdorModel);
    ui->amdorDataTable->setModel(p_proxyModel);

    connect(ui->amdorDataTable->selectionModel(),&QItemSelectionModel::selectionChanged,this,&AmdorWidget::updateFtPlots);
    connect(ui->amdorDataTable->selectionModel(),&QItemSelectionModel::selectionChanged,this,&AmdorWidget::configureButtons);
    connect(p_amdorModel,&AmdorDataModel::dataChanged,[=](){ ui->amdorPlot->updateData(p_amdorModel->graphData());});

    connect(ui->applyThresholdButton,&QToolButton::clicked,[=](){ p_amdorModel->applyThreshold(ui->thresholdBox->value());});
    connect(ui->addLinkButton,&QToolButton::clicked,this,&AmdorWidget::addLinkage);
    connect(ui->removeLinkButton,&QToolButton::clicked,this,&AmdorWidget::removeLinkage);
    connect(ui->exportButton,&QToolButton::clicked,this,&AmdorWidget::exportAscii);


    QPair<double,double> r = ad.frequencyRange();
    ui->amdorPlot->setPlotRange(r.first,r.second);

    configureButtons();
}

AmdorWidget::~AmdorWidget()
{
    delete ui;
}

void AmdorWidget::livePlotting(bool en)
{
    d_livePlot = en;
    if(d_livePlot)
        ui->amdorPlot->updateData(p_amdorModel->graphData());
}

void AmdorWidget::configureButtons()
{
    if(!d_editingPossible)
    {
        ui->addLinkButton->setEnabled(false);
        ui->removeLinkButton->setEnabled(false);
        ui->thresholdBox->setEnabled(false);
        ui->applyThresholdButton->setEnabled(false);
        ui->exportButton->setEnabled(false);
    }
    else
    {
        ui->thresholdBox->setEnabled(true);
        ui->applyThresholdButton->setEnabled(true);
        ui->exportButton->setEnabled(p_amdorModel->rowCount(QModelIndex()) > 0);

        QModelIndexList l = ui->amdorDataTable->selectionModel()->selectedRows();
        ui->addLinkButton->setEnabled(!l.isEmpty());
        ui->removeLinkButton->setEnabled(!l.isEmpty());
    }


}

void AmdorWidget::enableEditing(bool en)
{
    d_editingPossible = en;
    configureButtons();
}

void AmdorWidget::newRefScan(int num, int id, double i)
{
    p_amdorModel->addRefScan(num,id,i);
}

void AmdorWidget::newDrScan(int num, int id, double i)
{
    p_amdorModel->addDrScan(num, id, i);
    if(d_livePlot)
        ui->amdorPlot->updateData(p_amdorModel->graphData());
}

void AmdorWidget::updateFtPlots()
{
    QModelIndexList l = ui->amdorDataTable->selectionModel()->selectedRows();
    if(l.isEmpty())
        return;

    QModelIndex i = l.first();
    QModelIndex index = p_proxyModel->mapToSource(i);
    int refScan = p_amdorModel->data(p_amdorModel->index(index.row(),p_amdorModel->column(AmdorData::RefScanNum)),Qt::EditRole).toInt();
    int drScan = p_amdorModel->data(p_amdorModel->index(index.row(),p_amdorModel->column(AmdorData::DrScanNum)),Qt::EditRole).toInt();

    ui->refPlot->loadScan(refScan);
    ui->drPlot->loadScan(drScan);
}

void AmdorWidget::addLinkage()
{
    QModelIndexList l = ui->amdorDataTable->selectionModel()->selectedRows();
    if(l.isEmpty())
        return;

    int row = p_proxyModel->mapToSource(l.first()).row();
    int scanIndex = p_amdorModel->data(p_amdorModel->index(row,0),Qt::EditRole).toInt();
    p_amdorModel->addLinkage(scanIndex);
}

void AmdorWidget::removeLinkage()
{
    QModelIndexList l = ui->amdorDataTable->selectionModel()->selectedRows();
    if(l.isEmpty())
        return;

    int row = p_proxyModel->mapToSource(l.first()).row();
    int scanIndex = p_amdorModel->data(p_amdorModel->index(row,0),Qt::EditRole).toInt();
    p_amdorModel->removeLinkage(scanIndex);
}

void AmdorWidget::exportAscii()
{
    QDialog d(this);
    d.setWindowTitle(QString("AMDOR Export"));

    QList<QVector<QPointF>> sets = p_amdorModel->graphData();
    QList<QCheckBox*> cbList;
    QVBoxLayout *vbl = new QVBoxLayout(&d);

    QCheckBox *cb = new QCheckBox(QString("Unlinked"),&d);
    cb->setChecked(true);
    cbList.append(cb);
    vbl->addWidget(cb);

    for(int i=1; i<sets.size(); i++)
    {
        cb = new QCheckBox(QString("Set %1").arg(i),&d);
        cb->setChecked(true);
        cbList.append(cb);
        vbl->addWidget(cb);
    }

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d);
    vbl->addWidget(bb);
    connect(bb->button(QDialogButtonBox::Ok),&QPushButton::clicked,&d,&QDialog::accept);
    connect(bb->button(QDialogButtonBox::Cancel),&QPushButton::clicked,&d,&QDialog::reject);

    if(d.exec() == QDialog::Accepted)
    {
        QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
        QString p = s.value(QString("exportPath"),QDir::homePath()).toString();
        QString saveFile = QFileDialog::getSaveFileName(this,QString("AMDOR Export"),p+QString("/amdor%1.txt").arg(d_number));

        if(saveFile.isEmpty())
            return;

        QList<int> exportList;
        for(int i=0; i<cbList.size(); i++)
        {
            if(cbList.at(i)->isChecked())
                exportList.append(i);
        }

        if(!p_amdorModel->exportAscii(exportList,saveFile))
            QMessageBox::critical(this,QString("AMDOR Export Error"),QString("Could not open %1 for writing.\nPlease choose another location."));

        QString newPath = QFileInfo(saveFile).dir().absolutePath();
        s.setValue(QString("exportPath"),newPath);

    }
}
