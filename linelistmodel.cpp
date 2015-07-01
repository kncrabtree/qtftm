#include "linelistmodel.h"
#include <math.h>

LineListModel::LineListModel(QObject *parent) :
     QAbstractListModel(parent)
{
}

LineListModel::~LineListModel()
{
	//memory must be freed
	deleteAll();
}

void LineListModel::newEntry(const double center, const double spacing)
{
	//the spacing is a suggestion based on the frequency.
	//if there's already an entry, use its spacing rather than the suggestion
	double s = spacing;
	if(!lineList.isEmpty())
		s = lineList.at(lineList.size()-1)->splitting();

	//create a doppler pair object
	DopplerPair *dp = new DopplerPair(lineList.size(),center,s);

	//add the doppler pair, and notify the model that the number of rows is changing
	beginInsertRows(QModelIndex(),lineList.size(),lineList.size()+1);
	lineList.append(dp);
	endInsertRows();

	//send doppler pair to plot, and update model view etc
	emit dpCreated(dp);
	emit modelChanged();
}

DopplerPair *LineListModel::addLine(double center, double spacing, double amplitude)
{
    DopplerPair *dp = new DopplerPair(lineList.size(),center,spacing,amplitude);
	beginInsertRows(QModelIndex(),lineList.size(),lineList.size()+1);
	lineList.append(dp);
	endInsertRows();
	emit showDp(dp);
	emit modelChanged();
    return dp;
}

void LineListModel::addUniqueLine(double center, double spacing, double amplitude)
{
	for(int i=0;i<lineList.size();i++)
	{
		double c = lineList.at(i)->center();
		double sp = lineList.at(i)->splitting();

		if(fabs(c-center) < 0.0001 && fabs(spacing-sp) < 0.0001)
        {
            lineList.at(i)->setAmplitude(amplitude);
			return;
        }
	}

    DopplerPair *dp = new DopplerPair(lineList.size(),center,spacing,amplitude);
	beginInsertRows(QModelIndex(),lineList.size(),lineList.size()+1);
	lineList.append(dp);
	endInsertRows();
	emit showDp(dp);
	emit modelChanged();
}

QVariant LineListModel::data(const QModelIndex &index, int role) const
{
	//the only role we care about is the display role, and for that we should display the DP string
	if (!index.isValid())
		return QVariant();

	if (index.row() >= lineList.size())
		return QVariant();

	if (role == Qt::DisplayRole)
		return lineList.at(index.row())->toString();
	else
		return QVariant();
}

int LineListModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return lineList.size();
}

int LineListModel::rowCount() const
{
	return lineList.size();
}

QStringList LineListModel::getAllLines() const
{
	QStringList out;

	for(int i=0;i<lineList.size();i++)
		out.append(lineList.at(i)->toString());

	return out;
}

QStringList LineListModel::getMetaData() const
{
	QStringList out;
	for(int i=0; i<lineList.size(); i++)
        out.append(QString("%1\t%2\t%3\n").arg(lineList.at(i)->center(),0,'f',4).arg(lineList.at(i)->splitting(),0,'f',4).arg(lineList.at(i)->amplitude(),0,'f',4));
	return out;
}

QList<QPair<double,double> > LineListModel::getRanges() const
{
	//convert the doppler pairs to ranges (min and max frequency)
	QList<QPair<double,double> > out;
	for(int i=0;i<lineList.size();i++)
	{
		double min = lineList.at(i)->center() - lineList.at(i)->splitting();
		double max = lineList.at(i)->center() + lineList.at(i)->splitting();
		out.append(QPair<double,double>(min,max));
	}

    return out;
}

DopplerPair *LineListModel::getLine(int index)
{
    if(index >= 0 && index < lineList.size())
        return lineList.at(index);

    return nullptr;
}


void LineListModel::deleteDopplerPair(int num)
{
	//remove the appropriate doppler pair
	beginRemoveRows(QModelIndex(),num,num);
	delete lineList.takeAt(num);
	endRemoveRows();

	//renumber all other pairs
	for(int i=0; i<lineList.size(); i++)
	{
		if(lineList.at(i)->number() != i)
			lineList[i]->changeNumber(i);
	}

	if(lineList.size()==0)
		emit modelChanged();
}

void LineListModel::deleteAll()
{
	beginRemoveRows(QModelIndex(),0,lineList.size()-1);
	while(!lineList.isEmpty())
	{
		DopplerPair *d = lineList.takeFirst();
		d->detach();
		delete d;
	}
	endRemoveRows();

	emit modelChanged();
}

void LineListModel::dopplerChanged(DopplerPair *d)
{
	emit dataChanged(index(d->number()),index(d->number()));
}
