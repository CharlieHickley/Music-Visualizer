#include "display/display.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dataExtraction/dataExtraction.h"
#include "barLogic/barLogic.h"
#include "fft/fft.h"

static WAVInfo wav;
static DisplayState *display = NULL;
static AudioBarArray *audioBars = NULL;
static Clock *clock = NULL;
static SDL_AudioDeviceID audioDevice;


#define NUM_FRAMES 600

static void cleanupWav(void) {
    wavClose(&wav);
}

static void cleanupDisplay(void) {
    cleanDisplay(display, audioBars, clock);
}

static void cleanupAudio(void) {
    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();
}

int main(int argc, char **argv) {
    if(argc != 2){
        fprintf(stderr,"Wrong number of arguments supplied");
        exit(EXIT_FAILURE);
    }

    //songState state;
    wavOpen(argv[1],&wav);
    atexit(cleanupWav);

    //Opens and parses the wav file
    wavGetSamples(&wav);

    //Gets the number of windows in the song
    int numWindows = (wav.numSamples + HOPSIZE - 1) / HOPSIZE;

    //Gives the bars an initial values
    struct barState bStateMem;
    barState bstate = &bStateMem;
    initBarState(bstate);
    float window[SAMPLE_NUM] = {0.0f};
    float bins[SAMPLE_NUM/2];

    //Clock struct to keep track of the current loops time
    clock = createClock();

    //Display struct to hold window and renderer
    display = initDisplay();

    //Wraps the bars in a struct to hold additional data like colour
    audioBars = initAudioBars(bstate->nextBars);

    atexit(cleanupDisplay);

    //This is the structure required to play the wav file using
    // the SDL2 library
    SDL_AudioSpec spec = {
        .freq = wav.sampleRate,
        .format = AUDIO_F32,
        .channels = 1,
        .samples = SAMPLE_NUM,
        .callback = NULL
    };
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    atexit(cleanupAudio);   
    SDL_QueueAudio(audioDevice,wav.samples,wav.numSamples *sizeof(float));

    //This starts the song


    SDL_PauseAudioDevice(audioDevice,0);

    Uint32 totalAudioBytes = wav.numSamples * sizeof(float);

    //This represents the previous window that was processed
    int lastWindow = -1;

    //Runs while the queue is not empty and the program hasnt been quit
    while (display->running && SDL_GetQueuedAudioSize(audioDevice) > 0) {


        Uint32 remainingBytes = SDL_GetQueuedAudioSize(audioDevice);
        Uint32 playedBytes = totalAudioBytes - remainingBytes;

        int samplesPlayed = playedBytes / sizeof(float);
        int currentWindow = samplesPlayed / HOPSIZE;

        //Prevents an out of bounds access near the end of the song
        if (currentWindow >= numWindows) {
            currentWindow = numWindows - 1;
        }

        //Updates when enough samples have passed that it has moved to the next window
        if (currentWindow != lastWindow) {
            //Gets the window, passes it through fft then translates it to heights
            wavGetWindow(&wav, window, currentWindow);
            fft(window, SAMPLE_NUM, bins);
            getBarHeights(bins, bstate, &wav);
            //Updates to show which window has been processed
            lastWindow = currentWindow;
        }

        //Updates the GUI with the new bar state
        displayUpdate(display, audioBars, clock, bstate);
    }

    return EXIT_SUCCESS;
}
	