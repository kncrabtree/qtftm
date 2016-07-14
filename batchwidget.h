#ifndef BATCHWIDGET_H
#define BATCHWIDGET_H

#include <QWidget>
#include "singlescanwidget.h"
#include "scan.h"
#include "batchtablemodel.h"
#include <QLabel>
#include <QCheckBox>

namespace Ui {
class BatchWidget;
}

class BatchWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit BatchWidget(SingleScanWidget *ssw, QtFTM::BatchType type = QtFTM::Batch, QWidget *parent = nullptr);
	~BatchWidget();

	bool isEmpty();
	QList<QPair<Scan,bool> > getList() const { return btm.getList(); }
    bool sleep() const { return sleepCheckBox->isChecked(); }
    void setNumTests(int t) { d_numTests = t; }
    void disableCalibration();

public slots:
	void updateLabel();
	void addButtonCallBack();
	void insertButtonCallBack();
	void addCalCallBack();
	void insertCalCallBack();
	void toggleButtons();
	void clearButtonCallBack();
	void deleteRows();
	void editScan();
	void moveRowsUp();
	void moveRowsDown();
    void sortButtonCallBack();
	void parseFile();
	void saveFile();
    void setDrOnlyList(const QList<QPair<double,double>> l) { d_amdorDrOnlyList = l; }

signals:
	void scansChanged();
    void amdorDrOnly(double drf, double drp);
    void requestDrOnlyList();
	
private:
	Ui::BatchWidget *ui;
	SingleScanWidget *batchSsw;
	BatchTableModel btm;
	QtFTM::BatchType d_type;
	int d_numTests;
    bool d_calDisabled;

	Scan buildScanFromDialog(bool cal = false);
	bool isSelectionContiguous(QModelIndexList l);
	Scan parseLine(Scan defaultScan, QString line);
    void parseAmdorLine(QString line, double drp);
	QString writeScan(Scan thisScan, Scan ref = Scan());

    QList<QPair<double,double>> d_amdorDrOnlyList;

	QLabel *timeLabel;
    QCheckBox *sleepCheckBox;
};

#endif // BATCHWIDGET_H
