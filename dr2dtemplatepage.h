#ifndef DR2DTEMPLATEPAGE_H
#define DR2DTEMPLATEPAGE_H

#include <QWizardPage>

class Dr2dTemplatePage : public QWizardPage
{
	Q_OBJECT
public:
	explicit Dr2dTemplatePage(QWidget *parent = nullptr);
	int nextId() const;
	
signals:
	
public slots:
	
};

#endif // DR2DTEMPLATEPAGE_H
