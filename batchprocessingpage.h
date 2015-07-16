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
    void setType(QtFTM::BatchType type);

private:
    QtFTM::BatchType d_type;
    AutoFitWidget *batchAfw;
	
};

#endif // BATCHPROCESSINGPAGE_H
