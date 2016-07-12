#include "amdordatamodel.h"

AmdorDataModel::AmdorDataModel(AmdorData ad, QObject *parent) : QAbstractTableModel(parent), amdorData(ad)
{

}

void AmdorDataModel::addRefScan(int num, int id, double i)
{
    beginInsertRows(QModelIndex(),amdorData.numRows(),amdorData.numRows());
    amdorData.newRefScan(num,id,i);
    endInsertRows();
}

void AmdorDataModel::addDrScan(int num, int id, double i)
{
    beginInsertRows(QModelIndex(),amdorData.numRows(),amdorData.numRows());
    amdorData.newDrScan(num,id,i);
    endInsertRows();
}

void AmdorDataModel::addLinkage(int scanIndex)
{
    amdorData.addLinkage(scanIndex,true);
    emit dataChanged(index(scanIndex,5),index(scanIndex,5));
}

void AmdorDataModel::removeLinkage(int scanIndex)
{
    amdorData.removeLinkage(scanIndex);
    emit dataChanged(index(scanIndex,5),index(scanIndex,5));
}

QList<QVector<QPointF>> AmdorDataModel::graphData()
{
    return amdorData.graphData();
}

int AmdorDataModel::column(AmdorData::AmdorColumn c) const
{
    return amdorData.column(c);
}


int AmdorDataModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return amdorData.numRows();
}

int AmdorDataModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return amdorData.numColumns();
}

QVariant AmdorDataModel::data(const QModelIndex &index, int role) const
{
    return amdorData.modelData(index.row(),index.column(),role);
}

QVariant AmdorDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return amdorData.headerData(section,orientation,role);
}

Qt::ItemFlags AmdorDataModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}
