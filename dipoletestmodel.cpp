#include "dipoletestmodel.h"

#include <QDoubleSpinBox>

DipoleTestModel::DipoleTestModel(QObject *parent) :
    QAbstractListModel(parent)
{
    d_dipoles << 0.1 << 1.0 << 10.0;
}

QList<double> DipoleTestModel::dipoleList()
{
    QList<double> out = d_dipoles;
    std::stable_sort(out.begin(),out.end());
    std::unique(out.begin(),out.end());
    return out;
}

int DipoleTestModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d_dipoles.size();
}

QVariant DipoleTestModel::data(const QModelIndex &index, int role) const
{
    QVariant out;

    if(role == Qt::DisplayRole)
    {
        if(index.row() >=0 && index.row() < d_dipoles.size())
            out = QString("%1 D").arg(d_dipoles.at(index.row()),0,'f',1);
    }
    else if(role == Qt::EditRole)
    {
        if(index.row() >=0 && index.row() < d_dipoles.size())
            out = d_dipoles.at(index.row());
    }
    else if(role == Qt::SizeHintRole)
    {
        return QSize(0,20);
    }

    return out;
}

bool DipoleTestModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role == Qt::EditRole && index.row() >= 0 && index.row() < d_dipoles.size())
    {
        d_dipoles[index.row()] = value.toDouble();
        return true;
    }

    return false;
}

Qt::ItemFlags DipoleTestModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool DipoleTestModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    if(row < 0 || row >= d_dipoles.size())
    {
        beginInsertRows(parent,d_dipoles.size(),d_dipoles.size()+count);
        for(int i=0; i<count; i++)
            d_dipoles.append(1.0);
    }
    else
    {
        beginInsertRows(parent,row,row+count);
        for(int i=0; i<count; i++)
            d_dipoles.insert(row,1.0);
    }
    endInsertRows();

    return true;
}

bool DipoleTestModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(row < 0 || row+count > d_dipoles.size())
        return false;

    beginRemoveRows(parent,row,row+count);
    for(int i=0; i<count; i++)
        d_dipoles.removeAt(row);
    endRemoveRows();

    return true;
}



DipoleTestDelegate::DipoleTestDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}


QWidget *DipoleTestDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    QDoubleSpinBox *out = new QDoubleSpinBox(parent);
    out->setRange(0.1,10.0);
    out->setDecimals(1);
    out->setSingleStep(0.1);
    out->setSuffix(QString(" D"));
    out->setValue(index.model()->data(index).toDouble());
    return out;
}

void DipoleTestDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    qobject_cast<QDoubleSpinBox*>(editor)->setValue(index.model()->data(index,Qt::EditRole).toDouble());
}

void DipoleTestDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDoubleSpinBox *b = qobject_cast<QDoubleSpinBox*>(editor);
    b->interpretText();
    model->setData(index, b->value(),Qt::EditRole);
}

void DipoleTestDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}
