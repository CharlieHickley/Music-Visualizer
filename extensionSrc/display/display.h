#ifndef DISPLAY_H
#define DISPLAY_H
#include <SDL.h>
#include <stdbool.h>
#include <barLogic/barLogic.h>


#define FPS 60
#define FRAME_TIME (1000/FPS)
#define SCREEN_WIDTH (NUM_BARS*BAR_WIDTH + (NUM_BARS + 1)*BAR_GAP_WIDTH)
#define SCREEN_HEIGHT 750
#define BAR_WIDTH 25
#define BAR_GAP_WIDTH 3
#define EDGE_GAP_WIDTH (SCREEN_WIDTH - (NUM_BARS*BAR_WIDTH) - (NUM_BARS - 1)*BAR_GAP_WIDTH)
#define SMOOTH_RISE_FACTOR 0.6
#define SMOOTH_DROP_FACTOR 0.4

typedef struct {
    Uint32 loopStart;
    int loopTime;
} Clock;

typedef struct {
    SDL_Rect rect;
    float magnitude;
    int rval, gval, bval;
} AudioBar;

typedef struct {
    AudioBar* bars[NUM_BARS];
} AudioBarArray;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
} DisplayState;

Clock* createClock();
DisplayState* initDisplay();
AudioBarArray* initAudioBars(float* bars);
void displayUpdate(DisplayState* dstate, AudioBarArray* audioBars, Clock* clock, barState bstate);
void cleanDisplay(DisplayState* state, AudioBarArray* audioBars, Clock* clock);

#endif