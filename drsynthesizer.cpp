#include "drsynthesizer.h"
#include <math.h>

DrSynthesizer::DrSynthesizer(QObject *parent) :
    Synthesizer(parent)
{
    d_key = QString("drSynth");
}

void DrSynthesizer::initialize()
{
    Synthesizer::initialize();
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    s.beginGroup(d_key);
    s.beginGroup(d_subKey);

    s.endGroup();
    s.endGroup();
    s.sync();
}
