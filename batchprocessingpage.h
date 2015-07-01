#ifndef BATCHPROCESSINGPAGE_H
#define BATCHPROCESSINGPAGE_H

#include <QWizardPage>
#include "autofitwidget.h"
#include "abstractfitter.h"

class BatchProcessingPage : public QWizardPage
{
	Q_OBJECT
public:
    explicit BatchProcessingPage(AutoFitWidget *afw, QWidget *parent = nullptr);
	int nextId() const;
    bool validatePage();
	
signals:
    void fitter(AbstractFitter*);
	
public slots:

private:
    AutoFitWidget *batchAfw;
	
};

#endif // BATCHPROCESSINGPAGE_H
