#ifndef AMDORDRONLYMODEL_H
#define AMDORDRONLYMODEL_H

#include <QAbstractTableModel>

class AmdorDrOnlyModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    AmdorDrOnlyModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QList<QPair<double,double>> drList() const { return d_drList; }

signals:
    void modelChanged();

public slots:
    void addEntry(double f, double p);
    void insertEntry(double f, double p, int pos = -1);
    void removeEntries(QList<int> rows);

private:
    QList<QPair<double,double>> d_drList;
};

#endif // AMDORDRONLYMODEL_H
