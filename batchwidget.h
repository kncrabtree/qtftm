#ifndef BATCHWIDGET_H
#define BATCHWIDGET_H

#include <QWidget>
#include "singlescanwidget.h"
#include "scan.h"
#include "batchtablemodel.h"
#include <QLabel>

namespace Ui {
class BatchWidget;
}

class BatchWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit BatchWidget(SingleScanWidget *ssw, QWidget *parent = nullptr);
	~BatchWidget();

	bool isEmpty();
	QList<QPair<Scan,bool> > getList() const { return btm.getList(); }

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

signals:
	void scansChanged();
	
private:
	Ui::BatchWidget *ui;
	SingleScanWidget *batchSsw;
	BatchTableModel btm;
	Scan buildScanFromDialog(bool cal = false);
	bool isSelectionContiguous(QModelIndexList l);
	Scan parseLine(Scan defaultScan, QString line);
	QString writeScan(Scan thisScan, Scan ref = Scan());

	QLabel *timeLabel;
};

#endif // BATCHWIDGET_H
