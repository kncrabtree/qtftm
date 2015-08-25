#ifndef CATEGORYSETUPPAGE_H
#define CATEGORYSETUPPAGE_H

#include <QWizardPage>

#include "batchcategorize.h"

namespace Ui {
class CategorySetupPage;
}

class CategorySetupPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CategorySetupPage(QWidget *parent = 0);
    ~CategorySetupPage();

signals:
	void catTests(const QList<BatchCategorize::CategoryTest>);

private:
    Ui::CategorySetupPage *ui;

    // QWizardPage interface
public:
    void initializePage();
    bool validatePage();
    int nextId() const;
};

#endif // CATEGORYSETUPPAGE_H
