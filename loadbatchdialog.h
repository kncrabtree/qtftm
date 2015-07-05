#ifndef LOADBATCHDIALOG_H
#define LOADBATCHDIALOG_H

#include <QDialog>
#include "datastructs.h"

namespace Ui {
class LoadBatchDialog;
}

class LoadBatchDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit LoadBatchDialog(QWidget *parent = nullptr);
    ~LoadBatchDialog();

    QPair<QtFTM::BatchType,int> selection() const;
    double delay() const;
    int hpf() const;
    double exp() const;
    bool removeDC() const;
    bool padFid() const;
    
private:
    Ui::LoadBatchDialog *ui;
};

#endif // LOADBATCHDIALOG_H
