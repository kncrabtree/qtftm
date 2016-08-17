#include "peaklistwidget.h"
#include "ui_peaklistwidget.h"
#include "fitresult.h"
#include <QMenu>
#include <math.h>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>

PeakListWidget::PeakListWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PeakListWidget)
{
    ui->setupUi(this);

    p_plModel = new PeakListModel(this);
    p_proxy = new QSortFilterProxyModel(this);
    p_proxy->setSourceModel(p_plModel);
    p_proxy->setSortRole(Qt::EditRole);
    ui->peakListTable->setModel(p_proxy);

    connect(ui->peakListTable,&QTableView::clicked,this,&PeakListWidget::itemClicked);
    connect(ui->peakListTable,&QWidget::customContextMenuRequested,this,&PeakListWidget::contextMenu);
}

PeakListWidget::~PeakListWidget()
{
    delete ui;
}

void PeakListWidget::addScan(const Scan s)
{
    int num = s.number();

    if(num < 1)
	   return;

    FitResult res(num);

    if(res.freqAmpPairList().size() + res.freqAmpSingleList().size() == 0)
	   return;

    for(int i=0; i<res.freqAmpPairList().size(); i++)
	   addLine(num,res.freqAmpPairList().at(i).first,res.freqAmpPairList().at(i).second);

    for(int i=0; i<res.freqAmpSingleList().size(); i++)
	   addLine(num,res.freqAmpSingleList().at(i).first,res.freqAmpSingleList().at(i).second);
}

void PeakListWidget::removeScan(int num)
{
    if(num < 0)
        return;

    QList<int> rowsToRemove;
    for(int i=0; i<p_plModel->rowCount(QModelIndex());i++)
    {
        if(p_plModel->data(p_plModel->index(i,0),Qt::EditRole).toInt() == num)
            rowsToRemove.append(i);
    }

    if(rowsToRemove.isEmpty())
        return;

    p_plModel->removeLines(rowsToRemove);
}

void PeakListWidget::replaceScan(int num)
{
    if(num > 0)
    {
        removeScan(num);
        addScan(num);
    }
}

void PeakListWidget::addLine(int scanNum, double freq, double amp, QString comment, bool selectAdded)
{
    if(scanNum < 0)
        return;

    QModelIndex index = p_plModel->addLine(scanNum,freq,amp,comment);

    if(selectAdded)
    {
        ui->peakListTable->selectRow(p_proxy->mapFromSource(index).row());
    }

    if(index.row() == 0)
    {
        for(int i=0; i<p_plModel->columnCount(QModelIndex()); i++)
            ui->peakListTable->resizeColumnToContents(i);
    }
}

void PeakListWidget::removeSelectedLines()
{
    QModelIndexList l = ui->peakListTable->selectionModel()->selectedRows();
    if(l.isEmpty())
        return;

    QList<int> rows;
    for(int i=0; i<l.size(); i++)
        rows.append(p_proxy->mapToSource(l.at(i)).row());

    p_plModel->removeLines(rows);
}

void PeakListWidget::removeAllLines()
{
    if(QMessageBox::question(this,QString("Confirm Peak List Clear"),
                             QString("All lines will be removed from the peak list. Do you wish to proceed? This cannot be undone."),
                             QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
        return;

    p_plModel->removeAll();

}

void PeakListWidget::setTitle(QString t)
{
    if(t.isEmpty())
        ui->peakListTitle->setText(QString("Peak List"));
    else
        ui->peakListTitle->setText(QString("Peak List: %1").arg(t));
}

void PeakListWidget::itemClicked(QModelIndex index)
{
    int row = p_proxy->mapToSource(index).row();
    int scanNum = p_plModel->data(p_plModel->index(row,0),Qt::EditRole).toInt();

    if(scanNum > 0)
        emit scanSelected(scanNum);
}

void PeakListWidget::clearAll()
{
    p_plModel->removeAll();
}

void PeakListWidget::selectScan(int num)
{
    if(num < 1)
        return;

    QModelIndexList l = ui->peakListTable->selectionModel()->selectedRows();
    if(!l.isEmpty())
    {
        //if the currently selected item is from the same scan, don't change selection
        if(p_proxy->data(l.first(),Qt::EditRole).toInt() == num)
            return;
    }

    ui->peakListTable->clearSelection();

    if(num < 1 || p_proxy->rowCount(QModelIndex()) == 0)
        return;

    for(int i=0; i<p_proxy->rowCount(QModelIndex());i++)
    {
        if(p_proxy->data(p_proxy->index(i,0), Qt::EditRole).toInt() == num)
        {
            ui->peakListTable->selectRow(i);
            break;
        }
    }
}

void PeakListWidget::addUniqueLine(int scanNum, double freq, double amp, QString comment)
{
   int row = p_plModel->addUniqueLine(scanNum,freq,amp,comment);
   if(row >= 0)
   {
       ui->peakListTable->selectRow(p_proxy->mapFromSource(p_plModel->index(row,0)).row());
   }
}

void PeakListWidget::exportLinesToFile()
{
    if(p_proxy->rowCount(QModelIndex()) == 0)
        return;

    QString s = ui->peakListTitle->text().toLower();
    s.replace(QString(" "),QString(""));
    s.replace(QString(":"),QString("_"));

    QString fName = QFileDialog::getSaveFileName(this,QString("Peak List Export"),QString("~/%1.txt").arg(s));

    if(fName.isEmpty())
        return;

    QFile f(fName);

    if(!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this,QString("File Error"),QString("Coulf not open file %1 for writing. Please try again and choose a different file."),QMessageBox::Ok);
        return;
    }

    QTextStream t(&f);
    QString tab("\t");
    QString nl("\n");

    for(int i=0;i<p_proxy->rowCount(QModelIndex());i++)
    {
        if(i>0)
            t << nl;

        for(int j=0; j<p_proxy->columnCount(QModelIndex()); j++)
        {
            if(j>0)
                t << tab;
            t << p_proxy->data(p_proxy->index(i,j)).toString();
        }
    }

    t.flush();
    f.close();

    QDesktopServices::openUrl(QUrl(QString("file://%1").arg(f.fileName())));
}

void PeakListWidget::contextMenu(QPoint pos)
{
    QModelIndexList l = ui->peakListTable->selectionModel()->selectedRows();

    QMenu *menu = new QMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction *exportAct = menu->addAction(QString("Export list..."));
    if(p_proxy->rowCount(QModelIndex()) == 0)
	   exportAct->setEnabled(false);
    connect(exportAct,&QAction::triggered,this,&PeakListWidget::exportLinesToFile);

    menu->addSeparator();

    QAction *rsAct = menu->addAction(QString("Remove selected"));
    if(l.isEmpty())
        rsAct->setEnabled(false);
    connect(rsAct,&QAction::triggered,this,&PeakListWidget::removeSelectedLines);

    QAction *raAct = menu->addAction(QString("Remove all"));
    if(p_proxy->rowCount(QModelIndex()) == 0)
        raAct->setEnabled(false);
    connect(raAct,&QAction::triggered,this,&PeakListWidget::removeAllLines);

    menu->popup(ui->peakListTable->mapToGlobal(pos));
}


PeakListModel::PeakListModel(QObject *parent) : QAbstractTableModel(parent)
{

}


int PeakListModel::rowCount(const QModelIndex &parent) const
{
    //assuming we will never let the lists become dirrerent sizes...
    Q_UNUSED(parent)
    return d_scanList.size();
}

int PeakListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4;
}

QVariant PeakListModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
        return internalData(index.row(),index.column(),true);

    if(role == Qt::EditRole)
        return internalData(index.row(),index.column(),false);

    return QVariant();
}

bool PeakListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role == Qt::EditRole)
    {
        if(index.column() == 3)
        {
            if(index.row() >= 0 && index.row() < d_commentList.size())
            {
                d_commentList[index.row()] = value.toString();
                return true;
            }
        }
    }

    return false;
}

QVariant PeakListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch(section)
            {
            case 0:
                return QString("Scan");
                break;
            case 1:
                return QString("Frequency");
                break;
            case 2:
                return QString("Amplitude");
                break;
            case 3:
                return QString("Comment");
                break;
            }
        }
        else
            return QString::number(section+1);
    }

    return QVariant();
}

Qt::ItemFlags PeakListModel::flags(const QModelIndex &index) const
{
    if(index.column() == 3)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex PeakListModel::addLine(int scanNum, double freq, double amp, QString comment)
{
    int row = d_scanList.size();
    beginInsertRows(QModelIndex(),row,row);
    d_scanList.append(scanNum);
    d_freqList.append(freq);
    d_ampList.append(amp);
    d_commentList.append(comment);
    endInsertRows();

    return index(row,0);
}

int PeakListModel::addUniqueLine(int scanNum, double freq, double amp, QString comment)
{
    //first, search through list and see if this line is already there
    bool matchFound = false;
    int out = -1;
    for(int i=0;i<rowCount(QModelIndex());i++)
    {
        int rowScanNum = d_scanList.at(i);
        double rowFreq = d_freqList.at(i);

        if(rowScanNum == scanNum)
        {
            if(fabs(rowFreq - freq) < 0.0005) //same line; update amplitude and comment
            {
                matchFound = true;
                d_ampList[i] = amp;
                if(!comment.isEmpty())
                    d_commentList[i] = comment;
                out = i;
                break;
            }
        }
    }

    if(!matchFound)
        addLine(scanNum,freq,amp,comment);

    return out;
}

void PeakListModel::removeLines(QList<int> rows)
{
    std::sort(rows.begin(),rows.end());
    for(int i=rows.size(); i>=0; i--)
    {
        int row = rows.at(i);
        if(row < 0 || row > d_scanList.size())
            continue;

        beginRemoveRows(QModelIndex(),row,row);
        d_scanList.removeAt(row);
        d_freqList.removeAt(row);
        d_ampList.removeAt(row);
        d_commentList.removeAt(row);
        endRemoveRows();
    }
}

void PeakListModel::removeAll()
{
    beginRemoveRows(QModelIndex(),0,d_scanList.size()-1);
    d_scanList.clear();
    d_freqList.clear();
    d_ampList.clear();
    d_commentList.clear();
    endRemoveRows();
}

QVariant PeakListModel::internalData(int row, int column, bool str) const
{

    if(row < 0)
        return QVariant();

    if(str)
    {
        switch(column)
        {
        case 0:
            if(row < d_scanList.size())
                return QString::number(d_scanList.at(row));
            break;
        case 1:
            if(row < d_freqList.size())
                return QString::number(d_freqList.at(row),'f',3);
            break;
        case 2:
            if(row < d_ampList.size())
                return QString::number(d_ampList.at(row),'f',3);
            break;
        case 3:
            if(row < d_commentList.size())
                return d_commentList.at(row);
            break;
        }
    }
    else
    {
        switch(column)
        {
        case 0:
            if(row < d_scanList.size())
                return d_scanList.at(row);
            break;
        case 1:
            if(row < d_freqList.size())
                return d_freqList.at(row);
            break;
        case 2:
            if(row < d_ampList.size())
                return d_ampList.at(row);
            break;
        case 3:
            if(row < d_commentList.size())
                return d_commentList.at(row);
            break;
        }
    }

    return QVariant();


}
