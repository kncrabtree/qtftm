#ifndef PEAKLISTWIDGET_H
#define PEAKLISTWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "scan.h"

namespace Ui {
class PeakListWidget;
}

class PeakListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PeakListWidget(QWidget *parent = nullptr);
    ~PeakListWidget();

public slots:
    void addScan(const Scan s);
    void removeScan(int num);
    void replaceScan(int num);
    void addLine(int scanNum, double freq, double amp, QString comment = QString(""), bool selectAdded = false);
    void removeSelectedLine();
    void removeAllLines();
    void setTitle(QString t);
    void itemClicked(QTableWidgetItem *item);
    void clearAll();
    void selectScan(int num);
    void addUniqueLine(int scanNum, double freq, double amp, QString comment = QString(""));
    void exportLinesToFile();

    void contextMenu(QPoint pos);

signals:
    void scanSelected(int num);

private:
    Ui::PeakListWidget *ui;
};

class DoubleTableWidgetItem : public QTableWidgetItem
{

public:
    explicit DoubleTableWidgetItem(double num, int precision) : QTableWidgetItem(QString::number(num,'f',precision)) {}
    bool operator <(const QTableWidgetItem &other) const { return (text().toDouble() < other.text().toDouble()); }
};

#endif // PEAKLISTWIDGET_H
