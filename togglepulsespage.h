#ifndef TOGGLEPULSESPAGE_H
#define TOGGLEPULSESPAGE_H

#include <QWizardPage>

class TogglePulsesPage : public QWizardPage
{
	Q_OBJECT
public:
	explicit TogglePulsesPage(QWidget *parent = nullptr);
	int nextId() const;
	
signals:
	
public slots:
	
};

#endif // TOGGLEPULSESPAGE_H
