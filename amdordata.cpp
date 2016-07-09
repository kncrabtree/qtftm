#include "amdordata.h"

#include <QPair>
#include <QVector>
#include <QSet>

#include <boost/foreach.hpp>

class AmdorDataData : public QSharedData
{
public:
    AmdorDataData(const QList<double> fl) : allFrequencies(fl), matchThreshold(0.5), currentRefScan(-1), currentRefId(-1), currentRefInt(0.0) { sets.append(QSet<int>()); }

    QList<double> allFrequencies;
    QList<AmdorData::AmdorScanResult> resultList;
    QList<QSet<int>> sets;

    double matchThreshold;
    int currentRefScan;
    int currentRefId;
    double currentRefInt;
};

AmdorData::AmdorData(const QList<double> fl) : data(new AmdorDataData(fl))
{

}

AmdorData::AmdorData(const AmdorData &rhs) : data(rhs.data)
{

}

AmdorData &AmdorData::operator=(const AmdorData &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

AmdorData::~AmdorData()
{

}

void AmdorData::setMatchThreshold(const double t, bool recalculate)
{
    data->matchThreshold = t;

    if(recalculate)
    {
        //clear out pair lists and set lists; rebuild from resultList
    }
}

void AmdorData::newRefScan(int num, int id, double i)
{
    data->currentRefScan = num;
    data->currentRefInt = i;
    data->currentRefId = id;

    AmdorScanResult res;
    res.index = data->resultList.size();
    res.scanNum = num;
    res.refScanNum = num;
    res.ftId = id;
    res.refInt = i;
    res.drInt = i;
    res.isRef = true;
    res.set = -1;

    data->resultList.append(res);
}

bool AmdorData::newDrScan(int num, int id, double i)
{
    AmdorScanResult res;
    res.index = data->resultList.size();
    res.scanNum = num;
    res.refScanNum = data->currentRefScan;
    res.ftId = data->currentRefId;
    res.refInt = data->currentRefInt;
    res.drInt = i;
    res.drId = id;
    res.isRef = false;
    res.linked = (i < data->matchThreshold*data->currentRefInt);

    data->resultList.append(res);
    if(res.linked)
        addLinkage(data->resultList.size()-1,false);
    else
    {
        data->resultList[res.index].set = 0;
        data->sets[0].insert(res.index);
    }

    return res.linked;

}

void AmdorData::addLinkage(int scanIndex, bool checkUnlinked)
{
    if(scanIndex > data->resultList.size() || scanIndex < 0)
        return;

    data->resultList[scanIndex].linked = true;
    AmdorScanResult thisres = data->resultList.at(scanIndex);

    //determine set number
    int set = data->sets.size();
    for(int i=data->sets.size()-1; i>0; i--)
    {
        QSet<int> s = data->sets.at(i);
        Q_FOREACH(int id, s)
        {
            const AmdorScanResult &otherres = data->resultList.at(id);
            if(thisres.ftId == otherres.ftId || thisres.ftId == otherres.drId || thisres.drId == otherres.ftId || thisres.drId == otherres.drId)
            {
                set = i;
                break;
            }
        }
        if(set == i)
            break;
    }

    //if set is still data->sets.size(), then this linkage is not connected to any other known linkage, and we need to create a new set for it
    if(set == data->sets.size())
    {
        QSet<int> newSet { scanIndex };
        data->sets.append(newSet);
        data->resultList[scanIndex].set = set;
    }
    else
    {
        //we've added a new linkage to an existing set.
        //Now we need to check if this linkage connects this set to any other sets.
        //However, we already know that this new linkage cannot connect to any sets
        //AFTER the set we just added the linkage to (since we searched from the end).
        //So now we search all sets before this one to see if the new link matches

        //First, add the link to the existing set
        data->sets[set].insert(scanIndex);

        //look for another linked set
        int linkSet = set;
        for(int i = set-1; i>0; i--)
        {
            Q_FOREACH(int id, data->sets.at(i))
            {
                const AmdorScanResult &otherres = data->resultList.at(id);
                if(thisres.ftId == otherres.ftId || thisres.ftId == otherres.drId || thisres.drId == otherres.ftId || thisres.drId == otherres.drId)
                {
                    //found another link
                    linkSet = i;
                    break;
                }
                if(linkSet == i)
                    break;
            }
        }

        //If linkSet is still equal to set, then nothing more needs to be done.
        //However, if we found another link, then we need to merge the two sets and update the set IDs in the resultList
        if(linkSet != set)
        {
            QSet<int> oldSet = data->sets.takeAt(set);
            Q_FOREACH(int i, oldSet)
                data->resultList[i].set = linkSet;
            data->sets[linkSet].unite(oldSet);
        }
    }

    //If the user has manually assigned the linkage, then this may need to be removed from the unlinked set
    if(checkUnlinked)
        data->sets[0].remove(scanIndex);

    return;

}

void AmdorData::removeLinkage(int scanIndex)
{
    if(scanIndex > data->resultList.size() || scanIndex < 0)
        return;

    if(!data->resultList.at(scanIndex).linked)
        return;

    int oldSet = data->resultList[scanIndex].set;

    data->resultList[scanIndex].linked = false;
    data->resultList[scanIndex].set = 0;

    //pull link out of set and add it to the unlinked set
    data->sets[oldSet].remove(scanIndex);
    data->sets[0].insert(scanIndex);

    //Removing that link could potentially break the set into 2 separate groups
    //If so, we need to separate them out and reassign the set numbers
    QSet<int> startingSet(data->sets.at(oldSet));
    QSet<int> s1, s2;
    Q_FOREACH(int item, startingSet)
    {
        if(s1.isEmpty())
            s1.insert(item);
        else
        {
            bool added = false;
            Q_FOREACH(int item1, s1)
            {
                 if(data->resultList.at(item).ftId == data->resultList.at(item1).ftId || data->resultList.at(item).ftId == data->resultList.at(item1).drId ||
                         data->resultList.at(item).drId == data->resultList.at(item1).ftId || data->resultList.at(item).drId == data->resultList.at(item1).drId)
                 {
                     s1.insert(item);
                     added = true;
                     break;
                 }
                 if(added)
                     break;
            }

            //if we added this item to s1; see if it is also linked to an item in s2.
            //if so, that item in s2 needs to be pulled into s1
            //there can only be 1 such linkage by the way this algorithm works, because nothing in s2
            //is already linked to anything in s1
            if(added)
            {
                bool removed = false;
                Q_FOREACH(int item1, s2)
                {
                     if(data->resultList.at(item).ftId == data->resultList.at(item1).ftId || data->resultList.at(item).ftId == data->resultList.at(item1).drId ||
                             data->resultList.at(item).drId == data->resultList.at(item1).ftId || data->resultList.at(item).drId == data->resultList.at(item1).drId)
                     {
                         s2.remove(item1);
                         s1.insert(item1);
                         removed = true;
                         break;
                     }
                     if(removed)
                         break;
                }
            }
            else
            {
                //this item belongs in s2
                s2.insert(item);
            }
        }
    }

    //the larger set should take the place of the original set, and the other should be appended to the set list
    //if either set is empty, then nothing needs to be done at all
    if(s1.empty() || s2.empty())
        return;

    if(s1.size() > s2.size())
    {
        data->sets[oldSet] = s1;
        data->sets.append(s2);
    }
    else
    {
        data->sets[oldSet] = s2;
        data->sets.append(s1);
    }

    //update set ids in resultlist for the ones that are in the new set
    Q_FOREACH(int i, data->sets.last())
        data->resultList[i].set = data->sets.size()-1;


}

QList<QVector<QPointF> > AmdorData::graphData() const
{
    QList<QVector<QPointF>> out;

    for(int i=0; i<data->sets.size(); i++)
    {
        QVector<QPointF> v;
        Q_FOREACH(int index, data->sets.at(i))
        {
            double f1 = data->allFrequencies.at(data->resultList.at(index).ftId);
            double f2 = data->allFrequencies.at(data->resultList.at(index).drId);

            v.append(QPointF(f1,f2));
            v.append(QPointF(f2,f1));
        }
        out.append(v);
    }


    return out;

}

QVariant AmdorData::modelData(int row, int column, int role) const
{
    if(row < 0 || row >= numRows())
        return QVariant();

    if(column < 0 || column >= numColumns())

    if(role == Qt::DisplayRole)
    {
        switch(column)
        {
        case 0:
            return QString::number(row);
            break;
        case 1:
            return QString::number(data->resultList.at(row).scanNum);
            break;
        case 2:
            return QString::number(data->resultList.at(row).refScanNum);
            break;
        case 3:
            return QString::number(data->allFrequencies.at(data->resultList.at(row).ftId),'f',3);
            break;
        case 4:
            if(data->resultList.at(row).isRef)
                return QString("N/A");
            else
                return QString::number(data->allFrequencies.at(data->resultList.at(row).drId),'f',3);
            break;
        case 5:
            if(data->resultList.at(row).isRef)
                return QString("N/A");
            if(data->sets.at(0).contains(row))
                return QString("Unlinked");
            else
            {
                for(int i=1; i<data->sets.size(); i++)
                {
                    if(data->sets.at(i).contains(row))
                        return QString::number(i);
                }
            }
            break;
        default:
            return QVariant();

        }
    }

    return QVariant();
}

QVariant AmdorData::headerData(int index, Qt::Orientation o, int role) const
{
    if(role==Qt::DisplayRole)
    {
        if(o == Qt::Horizontal)
        {
            switch(index)
            {
            case 0:
                return QString("Index");
                break;
            case 1:
                return QString("Scan");
                break;
            case 2:
                return QString("Ref");
                break;
            case 3:
                return QString("FT (MHz)");
                break;
            case 4:
                return QString("DR (MHz)");
                break;
            case 5:
                return QString("Set");
                break;
            default:
                return QVariant();
            }
        }
        else
            return QVariant(index);
    }
    else if(role == Qt::ToolTipRole)
    {
        if(o == Qt::Horizontal)
        {
            switch(index)
            {
            case 0:
                return QString("Position of this scan in the original AMDOR experiment.");
                break;
            case 1:
                return QString("Number of this scan.");
                break;
            case 2:
                return QString("Reference scan (no DR) for this scan. If this scan is a reference, this will be the same as the scan number.");
                break;
            case 3:
                return QString("Cavity frequency");
                break;
            case 4:
                return QString("Double resonance frequency. For a reference scan, this will show \"N/A\"");
                break;
            case 5:
                return QString("The ID of the set to which this line belongs. Unlinked lines show up as \"unlinked\", and reference scans appear as \"N/A\"");
                break;
            }
        }
    }

    return QVariant();


}

int AmdorData::numRows() const
{
    return data->resultList.size();
}

int AmdorData::numColumns() const
{
    return 6;
}

