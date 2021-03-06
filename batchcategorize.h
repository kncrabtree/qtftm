#ifndef BATCHCATEGORIZE_H
#define BATCHCATEGORIZE_H

#include "batchmanager.h"

class BatchCategorize : public BatchManager
{
	Q_OBJECT
public:
	struct CategoryTest {
		QString key;
		QString name;
		bool categorize;
		QList<QVariant> valueList;
	};

	struct ScanResult {
        int index;
		int scanNum;
        QString testKey;
        QVariant testValue;
        int extraAttn;
		int attenuation;
        double ftMax;
	   QList<double> frequencies;
	   QList<double> intensities;
	   FitResult fit;
	};

    struct TestResult {
        int scanNum;
        QString key;
        QVariant value;
        FitResult result;
        int extraAttn;
        double ftMax;

        TestResult() : scanNum(-1), extraAttn(-1), ftMax(-1.0) {}
    };

    struct Candidate {
        double frequency;
        double intensity;
        double dipole;
    };

    struct CategoryStatus {
        int scanIndex;
        int scansTaken;
        bool lastWasSaturated;
        int currentTestIndex;
        int currentValueIndex;
        QString currentTestKey;
        QVariant currentTestValue;
        int currentAttn;
        int currentExtraAttn;
        TestResult bestDipoleResult;
        QList<Candidate> candidates;
	   QList<double> frequencies;
        QMap<QString,TestResult> resultMap;
        Scan scanTemplate;
        QString category;

        CategoryStatus() : scanIndex(0), scansTaken(0), lastWasSaturated(false), currentTestIndex(0), currentValueIndex(0), currentExtraAttn(0) {}
        void advance() { scanIndex++, scansTaken = 0, lastWasSaturated = false, currentTestIndex = 0, currentValueIndex = 0, currentExtraAttn = 0,
				currentTestKey = QString(""), currentTestValue = 0, bestDipoleResult = TestResult(), candidates.clear(), frequencies.clear(), resultMap.clear(),
                    scanTemplate = Scan(); category.clear(); }
    };

    explicit BatchCategorize(QList<QPair<Scan,bool>> scanList, QList<CategoryTest> testList, double freqWindow, AbstractFitter *ftr);
    explicit BatchCategorize(int num, AbstractFitter *ftr = new NoFitter());
	~BatchCategorize();

	// BatchManager interface
protected:
	void writeReport();
	void advanceBatch(const Scan s);
	void processScan(Scan s);
	Scan prepareNextScan();
	bool isBatchComplete();

private:
    const double d_lineMatchMaxDiff = 0.01;
    const double d_dcThresh = 0.2;
    const double d_magThresh = 0.75;
    double d_frequencyWindow;
	QList<QPair<Scan,bool>> d_templateList;
	QList<CategoryTest> d_testList;
    QList<ScanResult> d_resultList;
    QList<int> d_calScans;
	QVector<QPointF> d_scanData, d_calData;
    QMap<int,QString> d_loadLabelTextMap;

    CategoryStatus d_status;
	bool d_thisScanIsRef;
    int d_maxAttn;

    bool setNextTest();
    bool skipCurrentTest();
    bool configureScanTemplate();
    void getBestResult();
};

#endif // BATCHCATEGORIZE_H
