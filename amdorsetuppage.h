#ifndef AMDORSETUPPAGE_H
#define AMDORSETUPPAGE_H

#include <QWizardPage>

class QTableView;
class QPushButton;

class SingleScanWidget;
class BatchWidget;
class BatchManager;
class AmdorDrOnlyModel;

class AmdorSetupPage : public QWizardPage
{
    Q_OBJECT
public:
    AmdorSetupPage(SingleScanWidget *ssw, QWidget *parent = nullptr);

    // QWizardPage interface
public:
    void initializePage();
    bool validatePage();
    bool isComplete() const;
    int nextId() const;

signals:
    void batchManager(BatchManager*);
    void drOnly(QList<QPair<double,double>>);

private:
    BatchWidget *p_bw;
    QTableView *p_table;
    QPushButton *p_addButton, *p_insertButton, *p_removeButton;
    AmdorDrOnlyModel *p_model;

    void insertCallback();
    void removeCallback();
    void newEntryPopup(int pos = -1);
    void configureButtons();
};

#endif // AMDORSETUPPAGE_H
