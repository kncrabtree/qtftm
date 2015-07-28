#include "drsynthesizer.h"
#include <math.h>

DrSynthesizer::DrSynthesizer(QObject *parent) :
    Synthesizer(parent)
{
    d_key = QString("drSynth");
}

DrSynthesizer::~DrSynthesizer()
{

}

void DrSynthesizer::initialize()
{
    Synthesizer::initialize();
}
