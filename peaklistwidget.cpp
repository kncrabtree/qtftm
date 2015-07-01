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

    connect(ui->peakListTable,&QTableWidget::itemClicked,this,&PeakListWidget::itemClicked);
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
    for(int i=0; i<ui->peakListTable->rowCount();i++)
    {
        if(ui->peakListTable->item(i,0)->data(Qt::DisplayRole).toInt() == num)
            rowsToRemove.append(i);
    }

    if(rowsToRemove.isEmpty())
        return;

    ui->peakListTable->setSortingEnabled(false);
    for(int i=rowsToRemove.size()-1;i>=0;i--)
        ui->peakListTable->removeRow(i);
    ui->peakListTable->setSortingEnabled(true);
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
    int colNum = 0;

    ui->peakListTable->setSortingEnabled(false);

    int row = ui->peakListTable->rowCount();
    ui->peakListTable->setRowCount(row+1);

    QTableWidgetItem *sn = new QTableWidgetItem();
    sn->setData(Qt::DisplayRole,scanNum);
    sn->setTextAlignment(Qt::AlignRight);
    sn->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->peakListTable->setItem(row,colNum,sn);
    colNum++;

    QTableWidgetItem *f = new DoubleTableWidgetItem(freq,6);
    f->setTextAlignment(Qt::AlignRight);
    f->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->peakListTable->setItem(row,colNum,f);
    colNum++;

    QTableWidgetItem *a = new DoubleTableWidgetItem(amp,3);
    a->setTextAlignment(Qt::AlignRight);
    a->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->peakListTable->setItem(row,colNum,a);
    colNum++;

    QTableWidgetItem *c = new QTableWidgetItem(comment);
    c->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
    ui->peakListTable->setItem(row,colNum,c);

    if(selectAdded)
        ui->peakListTable->selectRow(row);

    ui->peakListTable->setSortingEnabled(true);
    for(int i=0; i<colNum; i++)
        ui->peakListTable->resizeColumnToContents(i);
}

void PeakListWidget::removeSelectedLine()
{
    QList<QTableWidgetItem*> selList = ui->peakListTable->selectedItems();
    if(!selList.isEmpty())
    {
        int row = selList.at(0)->row();
        ui->peakListTable->removeRow(row);
    }
}

void PeakListWidget::removeAllLines()
{
    if(QMessageBox::question(this,QString("Confirm Peak List Clear"),
                             QString("All lines will be removed from the peak list. Do you wish to proceed? This cannot be undone."),
                             QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
        return;

    while(ui->peakListTable->rowCount()>0)
        ui->peakListTable->removeRow(0);

}

void PeakListWidget::setTitle(QString t)
{
    if(t.isEmpty())
        ui->peakListTitle->setText(QString("Peak List"));
    else
        ui->peakListTitle->setText(QString("Peak List: %1").arg(t));
}

void PeakListWidget::itemClicked(QTableWidgetItem *item)
{
    int row = item->row();
    int scanNum = ui->peakListTable->item(row,0)->data(Qt::DisplayRole).toInt();

    if(scanNum > 0)
        emit scanSelected(scanNum);
}

void PeakListWidget::clearAll()
{
    ui->peakListTable->clear();
    ui->peakListTable->setRowCount(0);

    int colNum = 0;

    ui->peakListTable->setHorizontalHeaderItem(colNum,new QTableWidgetItem(QString("Scan")));
    colNum++;

    ui->peakListTable->setHorizontalHeaderItem(colNum,new QTableWidgetItem(QString("Frequency")));
    colNum++;

    ui->peakListTable->setHorizontalHeaderItem(colNum,new QTableWidgetItem(QString("Intensity")));
    colNum++;

    ui->peakListTable->setHorizontalHeaderItem(colNum,new QTableWidgetItem(QString("Comment")));
    colNum++;

    QHeaderView *hv = ui->peakListTable->verticalHeader();
    hv->setSectionResizeMode(QHeaderView::Fixed);
    ui->peakListTable->setVerticalHeader(hv);

    ui->peakListTable->horizontalHeader()->setStretchLastSection(true);
    ui->peakListTable->sortByColumn(0,Qt::AscendingOrder);
}

void PeakListWidget::selectScan(int num)
{
    if(num < 1)
        return;

    QList<QTableWidgetItem*> selList = ui->peakListTable->selectedItems();
    if(!selList.isEmpty())
    {
        int row = selList.at(0)->row();
        //if the currently selected item is from the same scan, don't change selection
        if(ui->peakListTable->item(row,0)->data(Qt::DisplayRole).toInt() == num)
            return;
    }

    ui->peakListTable->clearSelection();

    if(num < 1 || ui->peakListTable->rowCount() == 0)
        return;

    for(int i=0; i<ui->peakListTable->rowCount();i++)
    {
        if(ui->peakListTable->item(i,0)->data(Qt::DisplayRole).toInt() == num)
        {
            ui->peakListTable->selectRow(i);
            break;
        }
    }
}

void PeakListWidget::addUniqueLine(int scanNum, double freq, double amp, QString comment)
{
    //first, search through list and see if this line is already there
    bool matchFound = false;
    for(int i=0;i<ui->peakListTable->rowCount();i++)
    {
        int rowScanNum = ui->peakListTable->item(i,0)->data(Qt::DisplayRole).toInt();
        double rowFreq = ui->peakListTable->item(i,1)->data(Qt::DisplayRole).toDouble();

        if(rowScanNum == scanNum)
        {
            if(fabs(rowFreq - freq) < 0.0005) //same line; update amplitude and comment
            {
                matchFound = true;
                DoubleTableWidgetItem *it = new DoubleTableWidgetItem(amp,3);
                it->setTextAlignment(Qt::AlignRight);
                it->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
                ui->peakListTable->setItem(i,2,it);
                if(!comment.isEmpty())
                    ui->peakListTable->item(i,3)->setText(comment);
                ui->peakListTable->selectRow(i);
                break;
            }
        }
    }

    if(!matchFound)
        addLine(scanNum,freq,amp,comment,true);
}

void PeakListWidget::exportLinesToFile()
{
    if(ui->peakListTable->rowCount() == 0)
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

    for(int i=0;i<ui->peakListTable->rowCount();i++)
    {
        if(i>0)
            t << nl;

        for(int j=0; j<ui->peakListTable->columnCount(); j++)
        {
            if(j>0)
                t << tab;
            t << ui->peakListTable->item(i,j)->text();
        }
    }

    t.flush();
    f.close();

    QDesktopServices::openUrl(QUrl(QString("file://%1").arg(f.fileName())));
}

void PeakListWidget::contextMenu(QPoint pos)
{
    QTableWidgetItem *item = ui->peakListTable->itemAt(pos);

    QMenu *menu = new QMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction *exportAct = menu->addAction(QString("Export list..."));
    if(ui->peakListTable->rowCount() == 0)
	   exportAct->setEnabled(false);
    connect(exportAct,&QAction::triggered,this,&PeakListWidget::exportLinesToFile);

    menu->addSeparator();

    QAction *rsAct = menu->addAction(QString("Remove selected line"));
    if(!item)
        rsAct->setEnabled(false);
    connect(rsAct,&QAction::triggered,this,&PeakListWidget::removeSelectedLine);

    QAction *raAct = menu->addAction(QString("Clear all lines"));
    if(ui->peakListTable->rowCount() == 0)
        raAct->setEnabled(false);
    connect(raAct,&QAction::triggered,this,&PeakListWidget::removeAllLines);

    menu->popup(ui->peakListTable->mapToGlobal(pos));
}
