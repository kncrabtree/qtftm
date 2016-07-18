#ifndef AMDORBATCH_H
#define AMDORBATCH_H

#include "batchmanager.h"

#include "amdornode.h"

class AmdorBatch : public BatchManager
{
    Q_OBJECT
public:
    AmdorBatch(QList<QPair<Scan,bool>> templateList, QList<QPair<double,double>> drOnlyList, double threshold, double fw, double exc, int maxChildren, AbstractFitter *ftr);
    AmdorBatch(int num, AbstractFitter *ftr);
    ~AmdorBatch();

    struct AmdorSaveData {
        int scanNum;
        bool isCal;
        bool isRef;
        bool isValidation;
        int ftId;
        int drId;
        double intensity;
    };

    QList<double> allFrequencies();
    double matchThreshold();

    // BatchManager interface
protected:
    void writeReport();
    void advanceBatch(const Scan s);
    void processScan(Scan s);
    Scan prepareNextScan();
    bool isBatchComplete();

signals:
    void newRefScan(int,int,double);
    void newDrScan(int,int,double);

private:
    QList<QList<bool>> d_completedMatrix;
    QList<QList<Scan>> d_scanMatrix;
    QList<QPair<Scan,bool>> d_templateList;
    QVector<QPointF> d_drData, d_calData;
    QList<double> d_frequencies;
    int d_currentFtIndex;
    int d_currentDrIndex;
    double d_currentRefInt;
    double d_threshold;
    double d_frequencyWindow;
    double d_excludeRange;
    int d_maxChildren;
    Scan d_lastScan;
    Scan d_calScanTemplate;

    bool d_hasCal;
    bool d_calIsNext;
    int d_scansSinceCal;
    bool d_currentScanIsRef;
    bool d_currentScanIsVerification;
    AmdorNode *p_currentNode;
    QList<AmdorNode*> d_trees;
    QList<AmdorSaveData> d_saveData;
    QList<bool> d_loadCalList, d_loadRefList, d_loadVerificationList;
    QList<QPair<int,int>> d_loadIndices;
    int d_loadIndex;

    bool incrementIndices();
    bool nextTreeBranch();
    void resumeFromBranch();
};

#endif // AMDORBATCH_H
