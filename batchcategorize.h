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
		int scanNum;
		int scanIndex;
		int testIndex;
		int valueIndex;
		int attenuation;
		int lines;
		QString testKey;
		QList<QVariant> warnings;
		QList<QVariant> results;
	};

	explicit BatchCategorize(QList<QPair<Scan,bool>> scanList, QList<CategoryTest> testList, AbstractFitter *ftr);
	~BatchCategorize();

	// BatchManager interface
protected:
	void writeReport();
	void advanceBatch(const Scan s);
	void processScan(Scan s);
	Scan prepareNextScan();
	bool isBatchComplete();

private:
	QList<QPair<Scan,bool>> d_templateList;
	QList<CategoryTest> d_testList;
	QList<ScanResult> d_resultList, d_currentIndexResultList;
	QVector<QPointF> d_scanData, d_calData;

	int d_currentScanIndex;
	int d_currentTestIndex;
	int d_currentValueIndex;
	bool d_thisScanIsRef;
};

#endif // BATCHCATEGORIZE_H
