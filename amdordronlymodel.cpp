#include "amdordronlymodel.h"

#include <QSettings>
#include <QApplication>

AmdorDrOnlyModel::AmdorDrOnlyModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}



int AmdorDrOnlyModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d_drList.size();
}

int AmdorDrOnlyModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

QVariant AmdorDrOnlyModel::data(const QModelIndex &index, int role) const
{
    if(index.row() < 0 || index.row() >= d_drList.size() || index.column() < 0 || index.column() > columnCount(index))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0:
            return QString::number(d_drList.at(index.row()).first,'f',3);
            break;
        case 1:
            return QString::number(d_drList.at(index.row()).second,'f',3);
            break;
        }
    }
    else if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        case 0:
            return d_drList.at(index.row()).first;
            break;
        case 1:
            return d_drList.at(index.row()).second;
            break;
        }
    }

    return QVariant();
}

bool AmdorDrOnlyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.row() < 0 || index.row() >= d_drList.size() || index.column() < 0 || index.column() > columnCount(index))
        return false;

    if(role != Qt::EditRole)
        return false;

    bool ok = false;
    double d = value.toDouble(&ok);
    if(!ok)
        return false;

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("drSynth"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());

    if(index.column() == 0)
    {
        if(d >= s.value(QString("min"),1000.0).toDouble() && d <= s.value(QString("max"),1000000.0).toDouble())
        {
            d_drList[index.row()].first = d;
            emit dataChanged(index,index);
            s.endGroup();
            s.endGroup();
            return true;
        }
    }
    else
    {
        if(d >= s.value(QString("minPower"),-70.0).toDouble() && d <= s.value(QString("maxPower"),17.0).toDouble())
        {
            d_drList[index.row()].second = d;
            emit dataChanged(index,index);
            s.endGroup();
            s.endGroup();
            return true;
        }
    }

    s.endGroup();
    s.endGroup();
    return false;

}

QVariant AmdorDrOnlyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Vertical)
        return section;

    switch(section)
    {
    case 0:
        return QString("Frequency (MHz)");
        break;
    case 1:
        return QString("Power (dBm)");
        break;
    }

    return QVariant();
}

Qt::ItemFlags AmdorDrOnlyModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void AmdorDrOnlyModel::addEntry(double f, double p)
{
    beginInsertRows(QModelIndex(),d_drList.size(),d_drList.size());
    d_drList.append(qMakePair(f,p));
    endInsertRows();

    emit modelChanged();
}

void AmdorDrOnlyModel::insertEntry(double f, double p, int pos)
{
    if(pos < 0 || pos > d_drList.size())
        addEntry(f,p);
    else
    {
        beginInsertRows(QModelIndex(),pos,pos);
        d_drList.insert(pos,qMakePair(f,p));
        endInsertRows();
    }

    emit modelChanged();
}

void AmdorDrOnlyModel::removeEntries(QList<int> rows)
{
    std::sort(rows.begin(),rows.end());
    if(rows.isEmpty() || rows.first() < 0 || rows.last() >= d_drList.size())
        return;

    for(int i=rows.size()-1; i>=0; i--)
    {
        int row = rows.at(i);
        beginRemoveRows(QModelIndex(),row,row);
        d_drList.removeAt(row);
        endRemoveRows();
    }

    emit modelChanged();
}
