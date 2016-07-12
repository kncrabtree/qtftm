#ifndef AMDORDATA_H
#define AMDORDATA_H

#include <QSharedDataPointer>
#include <QPointF>
#include <QVariant>

class AmdorDataData;


class AmdorData
{
public:
    AmdorData(const QList<double> fl);
    AmdorData(const AmdorData &);
    AmdorData &operator=(const AmdorData &);
    ~AmdorData();

    struct AmdorScanResult {
        int index;
        int scanNum;
        int refScanNum;
        int ftId;
        int drId;
        double refInt;
        double drInt;
        bool linked;
        bool isRef;
        int set;

        AmdorScanResult() : index(-1), scanNum(-1), refScanNum(-1), ftId(-1), drId(-1), linked(false), isRef(false), set(0) {}
    };

    enum AmdorColumn {
        Index,
        RefScanNum,
        DrScanNum,
        FtFreq,
        DrFreq,
        SetNum
    };

    void setMatchThreshold(const double t, bool recalculate = false);
    void newRefScan(int num, int id, double i);
    bool newDrScan(int num, int id, double i);
    void addLinkage(int scanIndex, bool checkUnlinked = false);
    void removeLinkage(int scanIndex);

    QList<QVector<QPointF>> graphData() const;
    QPair<double,double> frequencyRange() const;

    //convenience interface for use with a QTableModel
    QVariant modelData(int row, int column, int role) const;
    QVariant headerData(int index, Qt::Orientation o, int role) const;
    int numRows() const;
    int numColumns() const;
    int column(AmdorColumn c) const;

private:
    QSharedDataPointer<AmdorDataData> data;
};

#endif // AMDORDATA_H
