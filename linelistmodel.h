#ifndef LINELISTMODEL_H
#define LINELISTMODEL_H

#include <QAbstractListModel>
#include <QAbstractItemModel>
#include "dopplerpair.h"

class LineListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit LineListModel(QObject *parent = nullptr);
	~LineListModel();

	void newEntry(const double center, const double spacing);
    DopplerPair* addLine(double center, double spacing, double amplitude = 0.0);
    void addUniqueLine(double center, double spacing, double amplitude = 0.0);
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int rowCount() const;

	QStringList getAllLines() const;
	QStringList getMetaData() const;
	QList<QPair<double, double> > getRanges() const;
    DopplerPair* getLine(int index);
	
signals:
	void dpCreated(DopplerPair *d);
	void showDp(DopplerPair *d);
	void modelChanged();
	
public slots:
	void deleteDopplerPair(int num);
	void deleteAll();
	void dopplerChanged(DopplerPair *d);

private:
	QList<DopplerPair*> lineList;
	
};

#endif // LINELISTMODEL_H
