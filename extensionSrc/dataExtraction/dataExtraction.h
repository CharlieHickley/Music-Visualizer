#ifndef DATAEXTRACTION_H
#define DATAEXTRACTION_H

#include <stdbool.h>
#include <stdio.h>

typedef struct{
    FILE *filePtr;
    int sampleRate;
    int numChannels;
    int numSampleBits;
    long numSamples;
    float *samples;
} WAVInfo;

typedef struct{
    int samplePosition;
    int totalSamples;
    bool finished;
} songState;

#define WINDOWSIZE 1024
#define HOPSIZE 64

// First the WAVInfo struct and songState struct are initialised
// First the wav file is opened, this also parses the header info
// Then the data is passed into the WAVInfo and songState struct
// Then the samples are gotten
// Then within the main file in a loop the windows are gotten iteratively
// Last the wav file is closed

void wavOpen(const char *filename, WAVInfo *wav);

void wavGetSamples(WAVInfo *wav);

void wavGetWindow(WAVInfo *wav, float *window, int index);

void wavApplyHanning(float *window);


void wavClose(WAVInfo *wav);

#endif