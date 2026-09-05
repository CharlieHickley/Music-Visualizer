#ifndef BARLOGIC_H
#define BARLOGIC_H

#include "../dataExtraction/dataExtraction.h"

#define NUM_BARS 64
#define MIN_FREQ 20.0f
#define MAX_FREQ 20000.0f

struct barState {
    float nextBars[NUM_BARS];
    float currentBars[NUM_BARS];
    float maximumVolume;   //Initialised to -60.0 dB
};

typedef struct barState *barState;

void initBarState(barState barState);
void getBarHeights(float *magnitudes, barState bars, WAVInfo *wavInfo);

#endif