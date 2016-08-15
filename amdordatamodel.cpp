#include "amdordatamodel.h"

AmdorDataModel::AmdorDataModel(AmdorData ad, QObject *parent) : QAbstractTableModel(parent), d_amdorData(ad)
{

}

void AmdorDataModel::addRefScan(int num, int id, double i)
{
    beginInsertRows(QModelIndex(),d_amdorData.numRows(),d_amdorData.numRows());
    d_amdorData.newRefScan(num,id,i);
    endInsertRows();
}

void AmdorDataModel::addDrScan(int num, int id, double i)
{
    beginInsertRows(QModelIndex(),d_amdorData.numRows(),d_amdorData.numRows());
    d_amdorData.newDrScan(num,id,i);
    endInsertRows();
}

void AmdorDataModel::addLinkage(int scanIndex)
{
    d_amdorData.addLinkage(scanIndex,true);
    emit dataChanged(index(0,0),index(rowCount(QModelIndex())-1,columnCount(QModelIndex())-1));
}

void AmdorDataModel::removeLinkage(int scanIndex)
{
    d_amdorData.removeLinkage(scanIndex);
    emit dataChanged(index(0,0),index(rowCount(QModelIndex())-1,columnCount(QModelIndex())-1));
}

QPair<QList<QVector<QPointF>>,QPointF> AmdorDataModel::graphData()
{
    return d_amdorData.graphData();
}

int AmdorDataModel::column(AmdorData::AmdorColumn c) const
{
    return d_amdorData.column(c);
}

AmdorData AmdorDataModel::amdorData() const
{
    return d_amdorData;
}

void AmdorDataModel::applyThreshold(double thresh)
{
    d_amdorData.setMatchThreshold(thresh,true);
    emit dataChanged(index(0,0),index(rowCount(QModelIndex())-1,columnCount(QModelIndex())-1));
}


int AmdorDataModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d_amdorData.numRows();
}

int AmdorDataModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d_amdorData.numColumns();
}

QVariant AmdorDataModel::data(const QModelIndex &index, int role) const
{
    return d_amdorData.modelData(index.row(),index.column(),role);
}

QVariant AmdorDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return d_amdorData.headerData(section,orientation,role);
}

Qt::ItemFlags AmdorDataModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}
