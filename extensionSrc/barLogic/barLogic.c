#include "barLogic.h"
#include "../dataExtraction/dataExtraction.h"

#include <math.h>

#define DECAY_CONSTANT 0.01f
#define DB_FLOOR -60.0f
#define SAMPLE_NO 1024
#define NO_BINS 512

// Convert magnitude to dB
#define CALC_DB(magnitude) (20.0f * log10f(magnitude + 1e-6f))


// Finds the range of bins a given bar index will cover using logarithmic spacing
// Returns the height of the bar [0.0, 1.0]
static float getBarHeight(int index, float *magnitudes, int sampleRate, barState bars, int *previousUpperBin) {
    // Calulcate the bounds of frequencies that the bar will cover
    // Uses logarithmic scaling to make lower frequencies more representative - aligns with the nature of human hearing
    float freqLow = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, (float)index / NUM_BARS);
    float freqHigh = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, (float)(index + 1) / NUM_BARS);
    
    // Find which bins the frequency range maps to
    int lowerBin = freqLow * SAMPLE_NO / sampleRate;
    int upperBin = freqHigh * SAMPLE_NO / sampleRate;

    // Ensure bar doesn't reuse bins already claimed by the previous bar
    if (lowerBin < *previousUpperBin) lowerBin = *previousUpperBin;
    if (upperBin <= lowerBin) upperBin = lowerBin + 1;
    if (upperBin >= NO_BINS) upperBin = NO_BINS - 1;    // Avoid use of an assert to prevent crashes
    if (lowerBin >= NO_BINS) lowerBin = NO_BINS - 1;

    *previousUpperBin = upperBin;

    float barHeight = 0.0f;

    // Average the magnitudes in the calculated bin range
    if (upperBin != lowerBin) {
        for (int i=lowerBin; i<upperBin; i++) {
            barHeight += magnitudes[i];
        }
        barHeight /= (upperBin - lowerBin);
    } else {
        barHeight = magnitudes[lowerBin];
    }

    // Convert bar height to dB
    // dB unit is logarithmically scaled which matches how humans perceive sound so our visualiser is more accurate
    float db = CALC_DB(barHeight);
    if (db < DB_FLOOR) db = DB_FLOOR;   // Ensure db is not set below DB_FLOOR

    // Update current max, decaying it if necessary
    if (db > bars->maximumVolume) {
        bars->maximumVolume = db;
    } else {
        bars->maximumVolume -= DECAY_CONSTANT;

        // Check decay dooes not overshoot current volume or DB_FLOOR
        if (bars->maximumVolume < db) bars->maximumVolume = db;
        if (bars->maximumVolume < DB_FLOOR) bars->maximumVolume = DB_FLOOR;
    }

    // Normalise dB to range [0, 1]
    float range = bars->maximumVolume - DB_FLOOR;
    if (range < 1.0f) range = 1.0f;  // Prevent division by near-zero
    float normalised = (db - DB_FLOOR) / range;

    return normalised;
}


// Public function which initialises the barState struct
void initBarState(barState barState){
    for (int i = 0; i < NUM_BARS; i ++) {
        barState->nextBars[i] = 0.0f;
        barState->currentBars[i] = 0.0f;
    }
    barState->maximumVolume = DB_FLOOR;
}

// Public function to be called after FFT has been used to calculate the magnitudes
// Takes in the magnitudes and maps them to bars
void getBarHeights(float *magnitudes, barState bars, WAVInfo *wavInfo) {
    int previousUpperBin = 0;
    for (int i = 0; i < NUM_BARS; i++) {
        bars->nextBars[i] = getBarHeight(i, magnitudes, wavInfo->sampleRate, bars, &previousUpperBin);
    }
}