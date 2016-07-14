#ifndef AMDORCONFIGPAGE_H
#define AMDORCONFIGPAGE_H

#include <QWizardPage>

class AutoFitWidget;
class AbstractFitter;

class AmdorConfigPage : public QWizardPage
{
    Q_OBJECT
public:
    AmdorConfigPage(AutoFitWidget* afw, QWidget *parent = nullptr);

    // QWizardPage interface
    bool validatePage();
    int nextId() const;

signals:
    void fitter(AbstractFitter*);

private:
    AutoFitWidget *p_afw;

};

#endif // AMDORCONFIGPAGE_H
