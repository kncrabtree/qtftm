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
		QList<QVariant> valueList;
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
	QList<QPair<Scan,bool>> d_scanList;
	QList<CategoryTest> d_testList;
};

#endif // BATCHCATEGORIZE_H
