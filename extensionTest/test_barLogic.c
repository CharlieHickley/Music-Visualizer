#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "barLogic/barLogic.h"



// Track the number of tests failed
static int failed = 0;

#define ASSERT(cond, msg) \
    if (cond) { printf("PASS: %s\n", msg); } \
    else { printf("FAIL: %s\n", msg); failed++; }

#define ASSERT_FLOAT_NEAR(a, b, eps, msg) \
    ASSERT(fabsf((a) - (b)) < (eps), msg)

// Helper functions
// Create a zeroed magnitude array
static void zeroMagnitudes(float *mag, int n) {
    for (int i = 0; i < n; i++) mag[i] = 0.0f;
}

// Returns which bar index a frequency should map to
static int freqToBar(float freq, int sampleRate) {
    for (int i = 0; i < NUM_BARS; i++) {
        float fLow  = MIN_FREQ * powf((float)MAX_FREQ / MIN_FREQ, (float)i       / NUM_BARS);
        float fHigh = MIN_FREQ * powf((float)MAX_FREQ / MIN_FREQ, (float)(i + 1) / NUM_BARS);
        if (freq >= fLow && freq < fHigh) return i;
    }
    return NUM_BARS - 1;
}



// Normal cases
void testInitialisation() {
    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    bool pass = true;
    for (int i = 0; i < NUM_BARS; i++) {
        if (bars->nextBars[i] != 0.0f || bars->currentBars[i] != 0.0f) {
            pass = false;
            break;
        }
    }
    if (bars->maximumVolume != -60.0f) pass = false;
    ASSERT(pass, "Initialisation - All fields zeroed and maximumVolume set to DB_FLOOR");
}

void testSingleNote() {
    // Set all magnitudes to 0 except ~1000Hz (bin 23) at 44100Hz sample rate
    float magnitudes[512];
    zeroMagnitudes(magnitudes, 512);
    magnitudes[23] = 1.0f;

    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    getBarHeights(magnitudes, bars, &wav);

    int expectedBar = freqToBar(1000.0f, 44100);

    ASSERT(bars->nextBars[expectedBar] > 0.5f, "Test single note - Expected bar is non-zero");

    bool othersZero = true;
    for (int i = 0; i < NUM_BARS; i++) {
        if (i != expectedBar && bars->nextBars[i] >= 0.2f) {
            othersZero = false;
            break;
        }
    }

    ASSERT(othersZero == true, "Test single note - All other bars are zero");
}

void testTwoNotes() {
    // Set all magnitudes to 0 except ~1000Hz (bin 23) and ~15000Hz (bin 348) at 44100Hz sample rate
    float magnitudes[512];
    zeroMagnitudes(magnitudes, 512);
    magnitudes[23] = 1.0f;
    magnitudes[348] = 1.0f;

    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    getBarHeights(magnitudes, bars, &wav);

    int firstBar = freqToBar(1000.0f, 44100);
    int secondBar = freqToBar(15000.0f, 44100);

    ASSERT(bars->nextBars[firstBar] > 0.5f && bars->nextBars[secondBar] > 0.5f, "Test two notes - Expected bars are non-zero");

    bool othersZero = true;
    for (int i = 0; i < NUM_BARS; i++) {
        if (i != firstBar && i != secondBar && bars->nextBars[i] >= 0.2f) {
            othersZero = false;
            break;
        }
    }

    ASSERT(othersZero == true, "Test two notes - All other bars are zero");
}

void testAllSame() {
    // Set magnitudes such that all frequencies are present
    // All bins have magnitude 1.0 and all bars should be roughly the same
    float magnitudes[512];
    for (int i = 0; i < 512; i++) magnitudes[i] = 1.0f;

    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    getBarHeights(magnitudes, bars, &wav);

    bool allPresent = true;
    float firstBar = bars->nextBars[0];
    for (int i = 0; i < NUM_BARS; i++) {
        if (fabsf(bars->nextBars[i] - firstBar) > 0.05f) {
            allPresent = false;
            break;
        }
    }
    ASSERT(allPresent == true, "Test all same - All frequencies are present with same value")
}

void testIncreasingAmplitude() {
    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    float magnitudes[512];

    // Increase magnitude and check maximumVolume grows 
    float prevMax = bars->maximumVolume;
    for (int frame = 1; frame <= 5; frame++) {
        zeroMagnitudes(magnitudes, 512);
        magnitudes[23] = 0.1f * frame;  // increasing signal at ~1000 Hz
        getBarHeights(magnitudes, bars, &wav);
    }
    ASSERT(bars->maximumVolume > prevMax, "Increasing amplitude - Maximum volume grows");

    // All bars should be in range [0, 1]
    bool inRange = true;
    for (int i = 0; i < NUM_BARS; i++) {
        if (bars->nextBars[i] < 0.0f || bars->nextBars[i] > 1.0f) {
            inRange = false;
            break;
        }
    }
    ASSERT(inRange, "Increasing amplitude - All bar heights remain in [0, 1]");
}


// Testing volume normalisation
void testDecay() {
    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    float magnitudes[512];

    // Loud sound to set a high maximumVolume
    zeroMagnitudes(magnitudes, 512);
    magnitudes[23] = 1.0f;
    getBarHeights(magnitudes, bars, &wav);
    float peakMax = bars->maximumVolume;

    // Silent frames to decay maximumVolume
    zeroMagnitudes(magnitudes, 512);
    for (int i = 0; i < 100; i++) {
        getBarHeights(magnitudes, bars, &wav);
    }
    ASSERT(bars->maximumVolume < peakMax, "Test decay - Maximum volume decreases after silence");

    // Check a quieter signal produces a visible bar after a period of quiet
    magnitudes[23] = 0.01f;
    getBarHeights(magnitudes, bars, &wav);
    int expectedBar = freqToBar(1000.0f, 44100);
    ASSERT(bars->nextBars[expectedBar] > 0.2f, "Test decay - Quiet signal visible after loud section fades");
}


// Testing edge cases
void testAllZeros() {
    float magnitudes[512];
    zeroMagnitudes(magnitudes, 512);

    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    getBarHeights(magnitudes, bars, &wav);

    bool valid = true;
    for (int i = 0; i < NUM_BARS; i++) {
        float v = bars->nextBars[i];
        if (v < 0.0f || v > 1.0f || v != v) {  // v != v catches NaN
            valid = false;
            break;
        }
    }
    ASSERT(valid, "All zeros - No NaN or out-of-range values");
}

void testLowestBar() {
    // Signal at the lowest frequency bin
    float magnitudes[512];
    zeroMagnitudes(magnitudes, 512);
    magnitudes[0] = 1.0f;

    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    getBarHeights(magnitudes, bars, &wav);

    // Should not crash and bar 0 result should be in range
    ASSERT(bars->nextBars[0] >= 0.0f && bars->nextBars[0] <= 1.0f,
           "Lowest bar - Handles when lowerBin == upperBin, result in [0, 1]");
}

void testHighestBar(){
    // Signal at the highest bin
    float magnitudes[512];
    zeroMagnitudes(magnitudes, 512);
    magnitudes[511] = 1.0f;

    struct barState barMemory;
    barState bars = &barMemory;
    initBarState(bars);

    WAVInfo wav = { .sampleRate = 44100 };
    getBarHeights(magnitudes, bars, &wav);

    ASSERT(bars->nextBars[NUM_BARS - 1] >= 0.0f && bars->nextBars[NUM_BARS - 1] <= 1.0f,
           "Highest bar - Can handle bin 511, result in [0, 1]");
}



int main(void) {
    printf("Running bar height calculation tests\n");

    testInitialisation();
    testSingleNote();
    testTwoNotes();
    testAllSame();
    testIncreasingAmplitude();
    testDecay();
    testAllZeros();
    testLowestBar();
    testHighestBar();

    return 0;
}