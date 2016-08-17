#ifndef PEAKLISTWIDGET_H
#define PEAKLISTWIDGET_H

#include <QWidget>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include "scan.h"

namespace Ui {
class PeakListWidget;
}

class PeakListModel;

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
    void removeSelectedLines();
    void removeAllLines();
    void setTitle(QString t);
    void itemClicked(QModelIndex index);
    void clearAll();
    void selectScan(int num);
    void addUniqueLine(int scanNum, double freq, double amp, QString comment = QString(""));
    void exportLinesToFile();

    void contextMenu(QPoint pos);

signals:
    void scanSelected(int num);

private:
    Ui::PeakListWidget *ui;
    PeakListModel *p_plModel;
    QSortFilterProxyModel *p_proxy;
};

class PeakListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PeakListModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QModelIndex addLine(int scanNum, double freq, double amp, QString comment = QString(""));
    int addUniqueLine(int scanNum, double freq, double amp, QString comment = QString(""));
    void removeLines(QList<int> rows);
    void removeAll();

private:
    QList<int> d_scanList;
    QList<double> d_freqList;
    QList<double> d_ampList;
    QList<QString> d_commentList;

    QVariant internalData(int row, int column, bool str) const;
};

#endif // PEAKLISTWIDGET_H
