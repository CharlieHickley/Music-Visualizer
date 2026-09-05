#include "display.h"
#include <stdlib.h>
#include <math.h>

//creates clock
Clock* createClock() {
    Clock* clock = malloc(sizeof(Clock));
    //Check the time at the start and end of the loop
    clock->loopStart = 0;
    clock->loopTime = 0;

    return clock;
}

DisplayState* initDisplay() {
    //initialise the library
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Init(SDL_INIT_AUDIO);    
    //create window and renderer
    DisplayState* dstate = malloc(sizeof(DisplayState));
    dstate->window = SDL_CreateWindow("Visualiser", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_ALWAYS_ON_TOP); //SDL_WINDOW_FULLSCREEN |
    dstate->renderer = SDL_CreateRenderer(dstate->window, -1, SDL_RENDERER_ACCELERATED);
    //initialise main loop
    dstate->running = true;

    return dstate;
}

//create an array of audiobar structs
//holds metadata, useful for colouring
AudioBarArray* initAudioBars(float* barMags) {
    AudioBarArray* barsWrapper = malloc(sizeof(AudioBarArray));

    for (int i = 0; i < NUM_BARS; i++) {
        //create and position rect of the audio bar
        AudioBar* bar = malloc(sizeof(AudioBar));
        //maths for the position of each bar
        bar->rect = (SDL_Rect){(EDGE_GAP_WIDTH/2 + i*BAR_WIDTH + i*BAR_GAP_WIDTH), SCREEN_HEIGHT, BAR_WIDTH, 0};
        bar->magnitude = barMags[i];

        //rainbow colouring
        bar->rval = (127 + 128 * sin(i * 0.1f + 0.0f));
        bar->gval = (127 + 128 * sin(i * 0.1f + 2.0f));
        bar->bval = (127 + 128 * sin(i * 0.1f + 4.0f));

        barsWrapper->bars[i] = bar;
    }
    return barsWrapper;
}

void displayUpdate(DisplayState* dstate, AudioBarArray* audioBars, Clock* clock, barState bstate) {
        //control framerate
        clock->loopStart = SDL_GetTicks();

        //clear the screen
        SDL_SetRenderDrawColor(dstate->renderer, 0, 0, 0, 255);
        SDL_RenderClear(dstate->renderer);
        //check for event
        SDL_Event e;
        //while event queue is not empty
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                //quit if user pressed close or quit
                case SDL_QUIT:
                    dstate->running = false;
                    break;
                //quit if q pressed
                case SDL_KEYDOWN:
                    if (e.key.keysym.sym == SDLK_q) {
                        dstate->running = false;
                    }
                    break;
                }
        }
        //drawing boxes
        for (int i = 0; i < NUM_BARS; i++) {
            //choose render colour
            SDL_SetRenderDrawColor(dstate->renderer, audioBars->bars[i]->rval, audioBars->bars[i]->gval, audioBars->bars[i]->bval, 255);

            //calculate difference between bars 
            float difference = bstate->nextBars[i] - bstate->currentBars[i];

            //smoothing factor, quick rise and slow descent
            if (difference > 0) {
                audioBars->bars[i]->rect.h = (int)-1*SCREEN_HEIGHT*(bstate->currentBars[i] + SMOOTH_RISE_FACTOR*difference);
            } else {
                audioBars->bars[i]->rect.h = (int)-1*SCREEN_HEIGHT*(bstate->currentBars[i] + SMOOTH_DROP_FACTOR*difference);
            }
            SDL_RenderFillRect(dstate->renderer, &audioBars->bars[i]->rect);

            //move onto next set of bars
            bstate->currentBars[i] = bstate->nextBars[i];
        }

        SDL_RenderPresent(dstate->renderer);

        clock->loopTime = SDL_GetTicks() - clock->loopStart;

        if (clock->loopTime < FRAME_TIME ) {
            SDL_Delay(FRAME_TIME - clock->loopTime);
        }
}

//clean the elemens of the display
//free elements
void cleanDisplay(DisplayState* dstate, AudioBarArray* audioBars, Clock* clock) {
    SDL_DestroyRenderer(dstate->renderer);
    SDL_DestroyWindow(dstate->window);
    SDL_Quit();

    free(dstate);
    //loop through the bars and free all of them
    for (int i = 0; i < NUM_BARS; i++) {
        free(audioBars->bars[i]);
    }
    free(audioBars);
    free(clock);

}