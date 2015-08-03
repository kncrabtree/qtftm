#ifndef DIPOLETESTMODEL_H
#define DIPOLETESTMODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>

#include <QList>

class DipoleTestModel : public QAbstractListModel
{
public:
    explicit DipoleTestModel(QObject *parent = nullptr);
    QList<double> dipoleList();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool insertRows(int row, int count, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent);

private:
    QList<double> d_dipoles;
};

class DipoleTestDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DipoleTestDelegate(QObject *parent = nullptr);

    // QAbstractItemDelegate interface
public:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // DIPOLETESTMODEL_H
