#include "voltagetestmodel.h"

#include <QSpinBox>
#include <QSettings>
#include <QApplication>

VoltageTestModel::VoltageTestModel(QObject *parent) :
    QAbstractListModel(parent)
{
    d_voltages << 700 << 850 << 1000 << 1150 << 1300;
}

QList<int> VoltageTestModel::voltageList()
{
    QList<int> out = d_voltages;
    std::stable_sort(out.begin(),out.end());
    std::unique(out.begin(),out.end());
    return out;
}



int VoltageTestModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d_voltages.size();
}

QVariant VoltageTestModel::data(const QModelIndex &index, int role) const
{
    QVariant out;

    if(role == Qt::DisplayRole)
    {
        if(index.row() >=0 && index.row() < d_voltages.size())
            out = QString("%1 V").arg(d_voltages.at(index.row()));
    }
    else if(role == Qt::EditRole)
    {
        if(index.row() >=0 && index.row() < d_voltages.size())
            out = d_voltages.at(index.row());
    }
    else if(role == Qt::SizeHintRole)
    {
        return QSize(0,20);
    }

    return out;
}

bool VoltageTestModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role == Qt::EditRole && index.row() >= 0 && index.row() < d_voltages.size())
    {
        d_voltages[index.row()] = value.toInt();
        return true;
    }

    return false;
}

Qt::ItemFlags VoltageTestModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool VoltageTestModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    if(row < 0 || row >= d_voltages.size())
    {
        beginInsertRows(parent,d_voltages.size(),d_voltages.size()+count);
        for(int i=0; i<count; i++)
            d_voltages.append(1000);
    }
    else
    {
        beginInsertRows(parent,row,row+count);
        for(int i=0; i<count; i++)
            d_voltages.insert(row,1000);
    }
    endInsertRows();

    return true;
}

bool VoltageTestModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(row < 0 || row+count > d_voltages.size())
        return false;

    beginRemoveRows(parent,row,row+count);
    for(int i=0; i<count; i++)
        d_voltages.removeAt(row);
    endRemoveRows();

    return true;
}

VoltageTestDelegate::VoltageTestDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QWidget *VoltageTestDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    QSpinBox *out = new QSpinBox(parent);
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("hvps"));
    s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
    int min = s.value(QString("min"),0).toInt();
    int max = s.value(QString("max"),2000).toInt();
    s.endGroup();
    s.endGroup();

    out->setRange(min,max);
    out->setSingleStep(25);
    out->setSuffix(QString(" V"));
    out->setValue(index.model()->data(index).toInt());
    return out;
}

void VoltageTestDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    qobject_cast<QSpinBox*>(editor)->setValue(index.model()->data(index,Qt::EditRole).toInt());
}

void VoltageTestDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox *b = qobject_cast<QSpinBox*>(editor);
    b->interpretText();
    model->setData(index, qobject_cast<QSpinBox*>(editor)->value(),Qt::EditRole);
}

void VoltageTestDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}
