#include "amdorconfigpage.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>

#include "autofitwidget.h"
#include "batchwizard.h"

AmdorConfigPage::AmdorConfigPage(AutoFitWidget *afw, QWidget *parent) :
    QWizardPage(parent)
{
    setTitle(QString("AMDOR Configuration"));
    setTitle(QString(""));

    QFormLayout *fl = new QFormLayout();

    QDoubleSpinBox *threshBox = new QDoubleSpinBox(this);
    threshBox->setDecimals(3);
    threshBox->setRange(0.001,0.999);
    threshBox->setSingleStep(0.05);
    threshBox->setToolTip(QString("The fractional decrease in intensity required for a positive DR match."));

    fl->addRow(QString("Match Threshold"),threshBox);

    QDoubleSpinBox *winBox = new QDoubleSpinBox(this);
    winBox->setDecimals(3);
    winBox->setRange(0.001,0.999);
    winBox->setSingleStep(0.05);
    winBox->setToolTip(QString("When multiple lines are found, the program chooses to analyze the line closest to the cavity frequency.\nIf that line falls further from the cavity frequency than this amount, it will be ignored."));
    winBox->setSuffix(QString(" MHz"));

    fl->addRow(QString("Frequency Window"),winBox);

    QDoubleSpinBox *excludeBox = new QDoubleSpinBox(this);
    excludeBox->setDecimals(3);
    excludeBox->setRange(0.0,10000.0);
    excludeBox->setSingleStep(50.0);
    excludeBox->setSuffix(QString(" MHz"));
    excludeBox->setToolTip(QString("No test will be performed if the difference between the DR and FT frequencies is less than this amount."));

    fl->addRow(QString("DR Exclusion Window"),excludeBox);

    QSpinBox *childrenBox = new QSpinBox(this);
    childrenBox->setRange(0,100);
    childrenBox->setSingleStep(1);
    childrenBox->setToolTip(QString("Maximum number of DR linkages to a single transition before the program begins exploring the linkage network.\nIf set to 0, no linkage networks will be explored; the scans will proceed in order like in DR correlation mode."));
    childrenBox->setSpecialValueText(QString("N/A"));

    fl->addRow(QString("Max Linkages"),childrenBox);

    QSpinBox *treeSizeBox = new QSpinBox(this);
    treeSizeBox->setRange(1,1000);
    treeSizeBox->setSingleStep(1);
    treeSizeBox->setToolTip(QString("Maximum number of DR linkages in a whole set. After reaching this value,\nthe program will stop exploring this network and will search for a new one.\nIf set to 1, no networks will be explored."));

    fl->addRow(QString("Max Set Size"),treeSizeBox);

    QCheckBox *calBox = new QCheckBox(this);
    calBox->setToolTip(QString("If checked, calibration scans will be performed periodically throughout acquisition."));

    fl->addRow(QString("Calibration?"),calBox);


    p_afw = new AutoFitWidget(afw->bufferGas().name,afw->delay(),afw->hpf(),afw->exp(),afw->zeroPad(),afw->temperature());

    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->addLayout(fl,0);
    vbl->addWidget(p_afw,1);

    setLayout(vbl);

    registerField(QString("amdorThreshold"),threshBox);
    registerField(QString("amdorWindow"),winBox);
    registerField(QString("amdorExclude"),excludeBox);
    registerField(QString("amdorChildren"),childrenBox);
    registerField(QString("amdorTreeSize"),treeSizeBox);
    registerField(QString("amdorCal"),calBox);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("wizard"));
    double mt = s.value(QString("amdorThreshold"),0.5).toDouble();
    double win = s.value(QString("amdorWindow"),0.1).toDouble();
    double exc = s.value(QString("amdorExclude"),100.0).toDouble();
    int ch = s.value(QString("amdorChildren"),3).toInt();
    int ts = s.value(QString("amdorTreeSize"),15).toInt();
    bool cal = s.value(QString("amdorCal"),true).toBool();

    s.endGroup();

    threshBox->setValue(mt);
    winBox->setValue(win);
    excludeBox->setValue(exc);
    childrenBox->setValue(ch);
    treeSizeBox->setValue(ts);
    calBox->setChecked(cal);

}


bool AmdorConfigPage::validatePage()
{
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(QString("wizard"));
    s.setValue(QString("amdorThreshold"),field(QString("amdorThreshold")).toDouble());
    s.setValue(QString("amdorWindow"),field(QString("amdorWindow")).toDouble());
    s.setValue(QString("amdorExclude"),field(QString("amdorExclude")).toDouble());
    s.setValue(QString("amdorChildren"),field(QString("amdorChildren")).toInt());
    s.setValue(QString("amdorTreeSize"),field(QString("amdorTreeSize")).toInt());
    s.setValue(QString("amdorCal"),field(QString("amdorCal")).toBool());
    s.endGroup();

    emit fitter(p_afw->toFitter());

    return true;
}

int AmdorConfigPage::nextId() const
{
    if(field(QString("amdorCal")).toBool())
        return BatchWizard::Page_AmdorCalSetup;

    return BatchWizard::Page_AmdorSetup;
}
