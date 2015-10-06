#ifndef LOADBATCHDIALOG_H
#define LOADBATCHDIALOG_H

#include <QDialog>
#include <memory>

#include "datastructs.h"

class AbstractFitter;

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
    AbstractFitter *fitter();
    
private:
    Ui::LoadBatchDialog *ui;
};

#endif // LOADBATCHDIALOG_H
