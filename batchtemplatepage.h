#ifndef BATCHTEMPLATEPAGE_H
#define BATCHTEMPLATEPAGE_H

#include <QWizardPage>

class BatchTemplatePage : public QWizardPage
{
	Q_OBJECT
public:
	explicit BatchTemplatePage(QWidget *parent = nullptr);
	int nextId() const;
	
signals:
	
public slots:
	
};

#endif // BATCHTEMPLATEPAGE_H
