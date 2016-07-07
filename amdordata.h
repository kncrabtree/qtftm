#ifndef AMDORDATA_H
#define AMDORDATA_H

#include <QSharedDataPointer>

class AmdorDataData;

class AmdorData
{
public:
    AmdorData();
    AmdorData(const AmdorData &);
    AmdorData &operator=(const AmdorData &);
    ~AmdorData();

    struct AmdorScanResult {
        int scanNum;
        int refScanNum;
        int ftId;
        int drId;
        double refInt;
        double drInt;
        bool linked;
        bool isRef;

        AmdorScanResult() : scanNum(-1), refScanNum(-1), ftId(-1), drId(-1), linked(false), isRef(false) {}
    };

    void setMatchThreshold(const double t, bool recalculate = false);

private:
    QSharedDataPointer<AmdorDataData> data;
};

#endif // AMDORDATA_H
