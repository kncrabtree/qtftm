#include "amdordata.h"

#include <QPoint>

class AmdorDataData : public QSharedData
{
public:
    AmdorDataData() : matchThreshold(0.5) {}

    QList<double> allFrequencies;
    QList<QList<QPoint>> pairLists;
    QList<QList<int>> setLists;
    QList<AmdorData::AmdorScanResult> resultList;

    double matchThreshold;
};

AmdorData::AmdorData() : data(new AmdorDataData)
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

