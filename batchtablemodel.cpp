#include "batchtablemodel.h"
#include <QSettings>
#include <QApplication>
#include <math.h>
#include <QListIterator>
#include "motordriver.h"

BatchTableModel::BatchTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
	//headers have a little extra space for padding
	headers << QString("  FTM Freq  ") << QString("  Shots  ") << QString("  Attn  ") << QString("  DR Freq  ")
		  << QString("  DR P  ") << QString("  DC V  ") << QString("  Is Cal?  ") << QString("  Pulses  ") << QString("  Magnet?  ");
	for(int i=0;i<headers.size();i++)
		setHeaderData(i,Qt::Horizontal,headers.at(i));

}

int BatchTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return scanList.size();
}

int BatchTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return headers.size();
}

QVariant BatchTableModel::data(const QModelIndex &index, int role) const
{
	if(role == Qt::TextAlignmentRole)
		return Qt::AlignCenter;

	if (role != Qt::DisplayRole)
	    return QVariant();

	if(index.row() >= scanList.size())
		return QVariant();

	switch(index.column())
	{
	case 0:
		return QString::number(scanList.at(index.row()).first.ftFreq(),'f',3);
		break;
	case 1:
		return scanList.at(index.row()).first.targetShots();
		break;
	case 2:
        if(scanList.at(index.row()).first.dipoleMoment()>0.0)
            return QString("%1 D").arg(QString::number(scanList.at(index.row()).first.dipoleMoment(),'f',2));
        else
            return QString("%1 dB").arg(scanList.at(index.row()).first.attenuation());
		break;
	case 3:
		return QString::number(scanList.at(index.row()).first.drFreq(),'f',3);
		break;
	case 4:
		return QString::number(scanList.at(index.row()).first.drPower(),'f',1);
		break;
	case 5:
		return QString::number(scanList.at(index.row()).first.dcVoltage());
	case 6:
		if(scanList.at(index.row()).second)
			return QString("Yes");
		else
			return QString("No");
		break;
	case 7:
	{
		//create string of 0s and 1s to represent pulse configuration
		QString out;
		PulseGenConfig pc = scanList.at(index.row()).first.pulseConfiguration();
		for(int i=0; i<pc.size(); i++)
		{
			if(pc.at(i).enabled)
				out.append(QChar('1'));
			else
				out.append(QChar('0'));
		}
		return out;
		break;
	}
    case 8:
        if(scanList.at(index.row()).first.magnet())
            return QString("Yes");
        else
            return QString("No");
        break;
	default:
		return QVariant();
		break;
	}

	return QVariant();
}

QVariant BatchTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{

	if (role == Qt::DisplayRole) //return row number or header
	{
		if(orientation == Qt::Vertical)
			return section+1;
		else
		{
			if(section >= headers.size())
				return QVariant();
			else
				return headers.at(section);
		}
	}

	if(role == Qt::ToolTipRole) //return a tool tip for header
	{
		if(orientation == Qt::Vertical)
			return QVariant();

		switch(section)
		{
		case 0:
			return QString("Cavity frequency (MHz)");
		case 1:
			return QString("Number of shots");
		case 2:
            return QString("Attenuation (dB) or dipole moment (D)");
		case 3:
			return QString("Double resonance frequency (MHz).\nOnly used if DR pulses are active");
		case 4:
			return QString("Double resonance power (dBm).\nOnly used if DR pulses are active");
		case 5:
			 return QString("Discharge voltage (V). Only used if DC pulses are active.");
		case 6:
			return QString("Whether the scan is a calibration line.\nCalibration scans are displayed with a special label on the bottom plot during the acquisition");
		case 7:
			return QString("Pulse configuration (1 = enabled, 0 = disabled).\nOrder is Gas, DC, MW, DR, Aux1, Aux2, Aux3, Aux4");
	   case 8:
            return QString("Whether the magnetic field will be altered during the scan.");
		default:
			return QVariant();
		}
	}

	return QVariant();
}

void BatchTableModel::addScan(Scan s, bool cal, int pos)
{
	if(pos < 0 || pos >= scanList.size())
	{
		//add scan to end of list
		beginInsertRows(QModelIndex(),scanList.size(),scanList.size());
		scanList.append(QPair<Scan,bool>(s,cal));
	}
	else
	{
		//put scan into list at the indicated position
		beginInsertRows(QModelIndex(),pos,pos);
		scanList.insert(pos,QPair<Scan,bool>(s,cal));
	}
	endInsertRows();
	emit modelChanged();

}

void BatchTableModel::addScans(QList<QPair<Scan, bool> > list, int pos)
{
	if(pos < 0 || pos >= scanList.size())
	{
		//append scans to end of list
		beginInsertRows(QModelIndex(),scanList.size(),scanList.size()+list.size()-1);
		scanList.append(list);
	}
	else
	{
		//insert scans into indicated position
		beginInsertRows(QModelIndex(),pos,pos+list.size()-1);
		for(int i=list.size(); i>0; i--)
			scanList.insert(pos,list.at(i-1));
	}
	endInsertRows();
	emit modelChanged();
}

void BatchTableModel::removeScans(QModelIndexList l)
{
	//make list of row numbers
	QList<int> removeList;
	for(int i=0; i<l.size();i++)
		removeList.append(l.at(i).row());

	//sort the list, and remove rows
	qSort(removeList);
	for(int i=removeList.size(); i>0; i--)
		removeRow(removeList.at(i-1),QModelIndex());

	emit modelChanged();
}

void BatchTableModel::moveRows(int first, int last, int delta)
{
	//make sure all movement is within valid ranges
	if(first + delta < 0 || last + delta >= scanList.size())
		return;

	//this bit of code is not intuitive! read docs on QAbstractItemModel::beginMoveRows() carefully!
	if(delta>0)
	{
		if(!beginMoveRows(QModelIndex(),first,last,QModelIndex(),last+2))
			return;
	}
	else
	{
		if(!beginMoveRows(QModelIndex(),first,last,QModelIndex(),first-1))
			return;
	}

	//make a copy of the rows to be moved
	QList<QPair<Scan,bool> > chunk = scanList.mid(first,last-first+1);

	//take out selected rows
	for(int i=0; i<last-first+1; i++)
		scanList.removeAt(first);


	//put rows in their new places (insert at same place from end to beginning)
	for(int i=chunk.size();i>0; i--)
	{
		if(delta>0)
			scanList.insert(first+1,chunk.at(i-1));
		else
			scanList.insert(first-1,chunk.at(i-1));
	}
	endMoveRows();

	emit modelChanged();
}

QPair<Scan, bool> BatchTableModel::getScan(int row) const
{
	if(row < 0 || row>= scanList.size())
		return QPair<Scan,bool>(Scan(),false);

	return scanList.at(row);
}

Scan BatchTableModel::getLastCalScan() const
{
	Scan out;
	for(int i=scanList.size(); i>0; i--)
	{
		if(scanList.at(i-1).second)
			return scanList.at(i-1).first;
	}
	return out;
}

int BatchTableModel::timeEstimate(QtFTM::BatchType type, int numRepeats) const
{
	if(scanList.isEmpty())
		return 0;

	int totalTime = 0;
	int totalShots = 0;
    double repRate = scanList.first().first.pulseConfiguration().repRate();

	if(type == QtFTM::Batch || type == QtFTM::Categorize)
	{
		totalShots += scanList.first().first.targetShots() * numRepeats;
		for(int i=1;i<scanList.size();i++)
		{
			if(fabs(scanList.at(i).first.ftFreq() - scanList.at(i-1).first.ftFreq()) > 1.0)
				totalTime += 10;
			totalShots += scanList.at(i).first.targetShots() * numRepeats;
		}
	}
	else if(type == QtFTM::DrCorrelation)
	{
        totalShots += scanList.first().first.targetShots();
        for(int i=1; i<scanList.size(); i++)
		{
            if((scanList.at(i).second || !qFuzzyCompare(scanList.at(i-1).first.ftFreq(),scanList.at(i).first.ftFreq())))
				totalTime += 10;

            for(int j=i+1; j<scanList.size(); j++)
            {
                if(scanList.at(j).second)
                    continue;

                totalShots += scanList.at(i).first.targetShots();
			}
		}
	}

	return totalTime + qRound(static_cast<double>(totalShots)/repRate);
}

void BatchTableModel::clear()
{
	removeRows(0,scanList.size(),QModelIndex());

	emit modelChanged();
}

void BatchTableModel::updateScan(int row, Scan s)
{
	if(row < 0 || row >= scanList.size())
		return;

	scanList.replace(row,QPair<Scan,bool>(s,scanList.at(row).second));

    emit dataChanged(index(row,0),index(row,headers.size()-1));
}

void BatchTableModel::sortByCavityPosition(SortOptions so, bool ascending)
{
	///TODO: rename this function; add ability to sort by shots (for dr correlation mode)
    //need to rearrange the real scans and evenly distribute cal scans.
    //should try to keep scans with the same FT frequency together, so cal scans may need to be shifted slightly to accommodate
    //first, collect cal scans for later use:
    QList<Scan> calScans;
    calScans.reserve(scanList.size());
    for(int i=0; i<scanList.size(); i++)
    {
        if(scanList.at(i).second) //if true, this is a calibration scan
            calScans.append(Scan(scanList.at(i).first));
    }

    //put all the other scans into a list.
    //make a list with the encoder positions and the indices of the scans in the list
    QList<Scan> scans;
    scans.reserve(scanList.size());
    QList<QPair<int,int> > positions; //the QPairs in the list store (encoderPos/freq*1000,originalIndex)
    positions.reserve(scanList.size());
    for(int i=0, index=0; i<scanList.size(); i++)
    {
       if(!scanList.at(i).second) //if the second element is false, this is NOT a cal scan, and we need to include it in the sort
       {
            scans.append(Scan(scanList.at(i).first));
            if(so == CavityPos)
                positions.append(QPair<int,int>(MotorDriver::calcRoughTune(scanList.at(i).first.ftFreq()).first,index));
            else
                positions.append(qMakePair((int)round(scanList.at(i).first.ftFreq()*1000.0),index));
            index++;
       }
    }

    //if there are 2 or fewer, then there's no point in doing this at all
    if(scans.size() < 2)
        return;

    //now, sort according to encoder position (see QtAlgorithms... for QPair, operator <() compares the first elements, then uses the second elements as tiebreakers)
    if(so != CalOnly)
        qSort(positions);

    //make a new list of scans in the order indicated by the positions list
    QList<Scan> sortedScans;
    sortedScans.reserve(scanList.size());
    if(ascending)
    {
        for(int i=0; i<positions.size() && i<scans.size(); i++)
            sortedScans.append(Scan(scans.at(positions.at(i).second)));
    }
    else
    {
        for(int i=qMin(positions.size()-1,scans.size()-1); i>=0; i--)
            sortedScans.append(Scan(scans.at(positions.at(i).second)));
    }

    //clear out old scan list, and add the sorted scans
    int listSize = scanList.size();
    clear();
    scanList.reserve(listSize);
    for(int i=0; i<sortedScans.size(); i++)
        addScan(Scan(sortedScans.at(i)));

    //if there are no cal scans, then we're done
    if(calScans.isEmpty())
        return;

    //if there's only 1 cal scan, put it at the end
    if(calScans.size() == 1)
    {
        addScan(Scan(calScans.at(0)),true);
        return;
    }

    //if there are only 2 cal scans, put one at beginning and the other at end
    if(calScans.size() == 2)
    {
        addScan(Scan(calScans.at(0)),true,0);
        addScan(Scan(calScans.at(1)),true);
        return;
    }

    //insert cal scans into scanList
    //however, we don't want to insert a cal scan in the middle of a group of scans taken at the same frequency if possible
    //The objective will be to evenly distribute cal scans, just shifting them a little if necessary, and to avoid "clumping" cal scans
    //To do this, we'll first make a list of "gaps" in the sorted scans, where the ft frequency changes by at least 500 kHz (which would force a rough tune)
    QList<int> gapIndices;
    gapIndices.reserve(sortedScans.size());
    for(int i=1; i<sortedScans.size(); i++)
    {
        if(fabs(sortedScans.at(i).ftFreq()-sortedScans.at(i-1).ftFreq()) >= 0.5)
            gapIndices.append(i);
    }

    //we'll put one cal scan at the beginning, another at the end, and try to evenly distribute the rest.
    double targetScansPerCal = (double)sortedScans.size()/((double)calScans.size()-1.0);

    //now we need to find the nearest gap indices and insert cal scans there, remembering that every time we insert a cal scan, the gap indices need to increase by 1
    int calScansAdded = 0;
    int lastGapIndex = 0;
    while(!calScans.isEmpty())
    {
        Scan s = calScans.takeFirst();

        //if the calScans list is now empty, we can put this at the end
        if(calScans.isEmpty())
            addScan(s,true);
        else if(calScansAdded == 0) //if calScansAdded is 0, we need to put this one at the beginning
            addScan(s,true,0);
        else
        {
            //calculate target index
            int targetIndex = (int)round(targetScansPerCal*(double)calScansAdded) + calScansAdded;

            //search through gap indices for the nearest gap
            int actualIndex = targetIndex; //if the search for a gap fails, we'll just force it in anyways
            for(int i=lastGapIndex; i<gapIndices.size()-1; i++)
            {
                //next time through on the while loop, we'll start at the next gap to avoid putting two cal scans next to each other
                //if the search fails, lastGapIndex will be at the end of the gapIndex list, and so targetIndex will be used on the next pass as well.
                lastGapIndex = i+1;

                int thisDiff = abs(targetIndex-(gapIndices.at(i)+calScansAdded));
                int nextDiff = abs(targetIndex-(gapIndices.at(i+1)+calScansAdded));

                //since the gaps are ordered in increasing index, this will find the lowest valid gap.
                //if there is no valid gap, then actualIndex will remain at its targetIndex value
                if(thisDiff < nextDiff)
                {
                    actualIndex = gapIndices.at(i)+calScansAdded;
                    break;
                }


            }

            addScan(s,true,actualIndex);
        }

        calScansAdded++;
    }

}

bool BatchTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
	if(row<0 || row+count > scanList.size() || scanList.isEmpty())
		return false;

	beginRemoveRows(parent,row,row+count-1);
	for(int i=0; i<count; i++)
		scanList.removeAt(row);
	endRemoveRows();

	return true;
}

bool BatchTableModel::removeRow(int row, const QModelIndex &parent)
{
	if(row < 0 || row >= scanList.size())
		return false;

	beginRemoveRows(parent,row,row);
	scanList.removeAt(row);
	endRemoveRows();

	return true;
}
