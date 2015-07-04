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
}
