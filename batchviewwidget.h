#ifndef BATCHVIEWWIDGET_H
#define BATCHVIEWWIDGET_H

#include <QWidget>
#include "batchmanager.h"
#include "abstractbatchplot.h"
#include <QThread>

class AmdorWidget;

namespace Ui {
class BatchViewWidget;
}

class BatchViewWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit BatchViewWidget(QtFTM::BatchType type, int num, AbstractFitter *af, QWidget *parent = nullptr);
    ~BatchViewWidget();

public slots:
    void process();
    void processingComplete(bool failure);
    
private:
    Ui::BatchViewWidget *ui;
    AbstractBatchPlot *batchPlot;

    QThread *batchThread;
    int d_number;
    QtFTM::BatchType d_type;
    int d_firstScan;
    int d_lastScan;

    AmdorWidget *p_amdorWidget;

signals:
    void metaDataChanged(int);
    void checkForMetaDataChanged(int);

};

#endif // BATCHVIEWWIDGET_H
