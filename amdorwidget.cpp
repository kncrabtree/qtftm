#include "amdorwidget.h"
#include "ui_amdorwidget.h"

AmdorWidget::AmdorWidget(AmdorData ad, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AmdorWidget), d_editingPossible(false)
{
    ui->setupUi(this);

    ui->refPlot->enableSelection(false);
    ui->drPlot->enableSelection(false);

    p_amdorModel = new AmdorDataModel(ad, this);
    p_proxyModel = new QSortFilterProxyModel(this);
    p_proxyModel->setSortRole(Qt::EditRole);
    p_proxyModel->setSourceModel(p_amdorModel);
    ui->amdorDataTable->setModel(p_proxyModel);

    QPair<double,double> r = ad.frequencyRange();
    ui->amdorPlot->setPlotRange(r.first,r.second);

    configureButtons();
}

AmdorWidget::~AmdorWidget()
{
    delete ui;
}

void AmdorWidget::configureButtons()
{
    if(!d_editingPossible)
    {
        ui->addLinkButton->setEnabled(false);
        ui->removeLinkButton->setEnabled(false);
        ui->thresholdBox->setEnabled(false);
        ui->applyThresholdButton->setEnabled(false);
    }
    else
    {
        ui->thresholdBox->setEnabled(true);
        ui->applyThresholdButton->setEnabled(true);

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
