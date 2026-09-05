#include "dataExtraction.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXWAVINT 32768.0f
#define PI 3.141592653589
#define CHUNKIDSIZE 4
#define FORMATSIZE 4
#define AUDIOFORMATSIZE 2
#define CHANNELSBYTES 2
#define BYTERATEBLOCKALIGNSIZE 6
#define SAMPLEBITSSIZE 2
#define MINHEADERSIZE 44

void wavOpen(const char *filename, WAVInfo *wav) {
    //open file in rb not r as it is a binary file not a text file
    wav->filePtr = fopen(filename, "rb");
    if (wav->filePtr == NULL) {
        fprintf(stderr, "The file cannot be opened\n");
        exit(1);
    }

    char chunkID[4];
    char format[4];
    uint32_t chunkSize;

    if (fread(chunkID, sizeof(char), 4, wav->filePtr) != 4 ||
        fread(&chunkSize, sizeof(uint32_t), 1, wav->filePtr) != 1 ||
        fread(format, sizeof(char), 4, wav->filePtr) != 4) {

        fprintf(stderr, "WAV file is not in a valid format\n");
        fclose(wav->filePtr);
        exit(1);
    }

    if (memcmp(chunkID, "RIFF", 4) != 0 || memcmp(format, "WAVE", 4) != 0) {
        fprintf(stderr, "File is not a valid WAV file\n");
        fclose(wav->filePtr);
        exit(1);
    }

    int foundFmt = 0;
    int foundData = 0;

    char subChunkID[4];
    uint32_t subChunkSize;

    while (fread(subChunkID, sizeof(char), 4, wav->filePtr) == 4 &&
           fread(&subChunkSize, sizeof(uint32_t), 1, wav->filePtr) == 1) {

        if (memcmp(subChunkID, "fmt ", 4) == 0) {
            // We found the fmt chunk
            if (subChunkSize < 16) {
                fprintf(stderr, "fmt chunk is not a valid size\n");
                fclose(wav->filePtr);
                exit(1);
            }

            uint16_t audioFormat;
            uint16_t numChannels;
            uint32_t sampleRate;
            uint32_t byteRate;
            uint16_t blockAlign;
            uint16_t bitsPerSample;

            if (fread(&audioFormat, sizeof(uint16_t), 1, wav->filePtr) != 1 ||
                fread(&numChannels, sizeof(uint16_t), 1, wav->filePtr) != 1 ||
                fread(&sampleRate, sizeof(uint32_t), 1, wav->filePtr) != 1 ||
                fread(&byteRate, sizeof(uint32_t), 1, wav->filePtr) != 1 ||
                fread(&blockAlign, sizeof(uint16_t), 1, wav->filePtr) != 1 ||
                fread(&bitsPerSample, sizeof(uint16_t), 1, wav->filePtr) != 1) {
                fprintf(stderr, "Could not read fmt chunk\n");
                fclose(wav->filePtr);
                exit(1);
            }

            if (audioFormat != 1) {
                fprintf(stderr, "WAV file must have PCM format\n");
                fclose(wav->filePtr);
                exit(1);
            }

            if (numChannels != 1 && numChannels != 2) {
                fprintf(stderr, "WAV file must be Mono or Stereo\n");
                fclose(wav->filePtr);
                exit(1);
            }

            if (bitsPerSample != 16) {
                fprintf(stderr, "Can only support 16-bit WAV files\n");
                fclose(wav->filePtr);
                exit(1);
            }

            wav->numChannels = numChannels;
            wav->sampleRate = sampleRate;
            wav->numSampleBits = bitsPerSample;

            foundFmt = 1;

            uint32_t sizeReadFromFmt = 16;
            
            // there may be extra data at the end of the fmt that we do not need therefore we skip it if that is the case
            if (subChunkSize > sizeReadFromFmt) {
                fseek(wav->filePtr, subChunkSize - sizeReadFromFmt, SEEK_CUR);
            }

            // This skips the padding byte at the end of the chunk if one is there, as we know the chunk will be padded
            // to an even value therefore if it is not even we must add 1 and if it is even we add 0 which is done by adding
            // subChunkSize % 2
            fseek(wav->filePtr, subChunkSize % 2, SEEK_CUR);
        }

        else if (memcmp(subChunkID, "data", 4) == 0) {
            // we found the data chunk
            if (!foundFmt) {
                fprintf(stderr, "Data chunk before fmt chunk, therefore the WAV file is invalid\n");
                fclose(wav->filePtr);
                exit(1);
            }

            int numSampleBytes = wav->numSampleBits / 8;
            int sampleBytesPerFrame = wav->numChannels * numSampleBytes;

            if (sampleBytesPerFrame == 0) {
                fprintf(stderr, "WAV file has invalid format values\n");
                fclose(wav->filePtr);
                exit(1);
            }

            wav->numSamples = subChunkSize / sampleBytesPerFrame;

            foundData = 1;

            break;
        }

        else {
            // we didnt find the data chunk or the fmt chunk therefore we skip this chunk
            // the same modulus trick as earlier is done due to the same padding reasons as earlier
            fseek(wav->filePtr, subChunkSize + (subChunkSize % 2), SEEK_CUR);
        }
    }

    if (!foundData) {
        fprintf(stderr, "Could not find the data chunk\n");
        fclose(wav->filePtr);
        exit(1);
    }
}

void wavGetSamples(WAVInfo *wav) {
    wav->samples = malloc(sizeof(float) * wav->numSamples);
    if (wav->samples == NULL) {
        exit(1);
    }

    // we now need to be able to store the data from the file
    int numElements = wav->numSamples * wav->numChannels;
    int elementSize = sizeof(int16_t) * numElements;
    int16_t *buffer = malloc(elementSize);
    if (buffer == NULL) {
        free(wav->samples);
        exit(1);
    }

    // we must ensure that the file is large enough to hold header information
    long savedPosition = ftell(wav->filePtr);
    fseek(wav->filePtr, 0, SEEK_END);
    long fileSize = ftell(wav->filePtr);
    if (savedPosition + elementSize > fileSize) {
        free(wav->samples);
        free(buffer);
        fprintf(stderr, "The file is not large enough to contain header information\n");
        exit(1);
    }

    //we must ensure that we are at the start of the data and passed the header file
    fseek(wav->filePtr, savedPosition, SEEK_SET);
    fread(buffer, sizeof(int16_t), numElements, wav->filePtr);

    if (wav->numChannels == 1) {
        for (int i = 0; i < wav->numSamples; i++) {
            (wav->samples)[i] = buffer[i] / MAXWAVINT;
        }
    } 
    else if (wav->numChannels == 2) {
        for (int i = 0; i < wav->numSamples; i++) {
            // for a stereo we must average the values of each of the channels
            (wav->samples)[i] = ((float)(buffer[i * 2] + (float)buffer[i * 2 + 1]) / 2.0f) / MAXWAVINT;
        }
    }
    else {
        fprintf(stderr, "The wav file is not in the correct format, it must be Mono or Stereo");
        free(wav->samples);
        free(buffer);
        exit(1);
    }

    free(buffer);
}

void wavGetWindow(WAVInfo *wav,float *window, int index) {
    int position = index * HOPSIZE;
    if (position >= wav->numSamples) {
        // case that we are trying to index outside of the number of samples
        return;
    }
    
    int numLeft = wav->numSamples - position;
    if (numLeft >= WINDOWSIZE) {
        memcpy(window, wav->samples + position, WINDOWSIZE * sizeof(float));
    }
    else {
        memcpy(window, wav->samples + position, numLeft * sizeof(float));
        memset(window + numLeft, 0, (WINDOWSIZE - numLeft) * sizeof(float));
    }

    wavApplyHanning(window);
}

// The hanning function is needed to ensure that the processing of the data occurs correctly
// without it the start of the wave and the end of the wave would be not at teh same point
// which is a frequency value of infinite as amplitude changes lots in no time
void wavApplyHanning(float *window) {
    float constant = 2.0f * PI / (WINDOWSIZE - 1);
    for (int i = 0; i < WINDOWSIZE; i++) {
        window[i] *= 0.5f * (1- cosf(constant * i));
    }
}

void wavClose(WAVInfo *wav) {
    // must check that there is a file stored to close and that the wav info file
    // passed has been made to avoid errors.
    if (wav != NULL && (wav->filePtr != NULL)) {
        fclose(wav->filePtr);
    }
    if (wav != NULL && (wav->samples != NULL)) {
        free(wav->samples);
    }
}