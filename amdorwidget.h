#ifndef AMDORWIDGET_H
#define AMDORWIDGET_H

#include <QWidget>

#include <QSortFilterProxyModel>

#include "amdordatamodel.h"

namespace Ui {
class AmdorWidget;
}

class AmdorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AmdorWidget(AmdorData ad, int num, QWidget *parent = 0);
    ~AmdorWidget();

    void livePlotting(bool en);

public slots:
    void configureButtons();
    void enableEditing(bool en);
    void newRefScan(int num, int id, double i);
    void newDrScan(int num, int id, double i);
    void updateFtPlots();
    void goToLastScan();

    void addLinkage();
    void removeLinkage();
    void exportAscii();

private:
    Ui::AmdorWidget *ui;

    AmdorDataModel *p_amdorModel;
    QSortFilterProxyModel *p_proxyModel;

    bool d_editingPossible;
    bool d_livePlot;
    const int d_number;
};

#endif // AMDORWIDGET_H
