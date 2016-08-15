#ifndef AMDORDATAMODEL_H
#define AMDORDATAMODEL_H

#include <QAbstractTableModel>

#include "amdordata.h"

class AmdorDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    AmdorDataModel(AmdorData ad, QObject *parent = nullptr);

    void addRefScan(int num, int id, double i);
    void addDrScan(int num, int id, double i);
    void addLinkage(int scanIndex);
    void removeLinkage(int scanIndex);
    QPair<QList<QVector<QPointF> >, QPointF> graphData();
    int column(AmdorData::AmdorColumn c) const;
    AmdorData amdorData() const;

public slots:
    void applyThreshold(double thresh);

private:
    AmdorData d_amdorData;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // AMDORDATAMODEL_H
