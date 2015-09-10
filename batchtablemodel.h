#ifndef BATCHTABLEMODEL_H
#define BATCHTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "scan.h"

/***** COLUMN DEFINITIONS ******
 * 0: FTM Freq
 * 1: Shots
 * 2: Attenuation
 * 3: DR Freq
 * 4: DR Power
 * 5: DC Voltage
 * 6: Is Cal
 * 7: Pulse string
 *******************************/

class BatchTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit BatchTableModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	void addScan(Scan s, bool cal = false, int pos = -1);
	void addScans(QList<QPair<Scan,bool> > list, int pos = -1);
	void removeScans(QModelIndexList l);
	void moveRows(int first, int last, int delta);
	QPair<Scan,bool> getScan(int row) const;
	Scan getLastCalScan() const;
	int timeEstimate(QtFTM::BatchType type = QtFTM::Batch, int numRepeats = 1) const;

	QList<QPair<Scan,bool> > getList() const { return scanList; }

    enum SortOptions {
        Frequency,
        CavityPos,
        CalOnly
    };
	
signals:
	void modelChanged();
	
public slots:
	void clear();
	void updateScan(int row, Scan s);
    void sortByCavityPosition(SortOptions so = CalOnly, bool ascending = true);

private:
	QList<QPair<Scan,bool> > scanList;
	QStringList headers;

	bool removeRows(int row, int count, const QModelIndex &parent);
	bool removeRow(int row, const QModelIndex &parent);


};

#endif // BATCHTABLEMODEL_H
