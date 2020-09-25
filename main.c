#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "chip8.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <Scale> <Delay> <ROM>\n", argv[0]);
		exit(1);
	}

    int videoScale = atoi(argv[1]);
    int cycleDelay = atoi(argv[2]);
    char const* rom = argv[3];

    int video_width = 64;
    int video_height = 32;
    
    struct MultimediaLayer* mult = makeMultimediaLayer("CHIP-8", video_width * videoScale, video_height * videoScale, video_width, video_height);
    struct chip8* chip8 = init();
    load(chip8, rom);

    int videoPitch = sizeof(chip8->video[0]) * video_width;

    clock_t lastCycleTime = clock();
    bool run = true;

    while(run) {
        run = processInput(mult, chip8->keypad);

        clock_t currentTime = clock();
        float dt = (float)(currentTime - lastCycleTime) / CLOCKS_PER_SEC;

        if(dt > cycleDelay) {
            lastCycleTime = currentTime ;
            cycle(chip8);
            updateMultimediaLayer(mult, chip8->video, videoPitch);
        }
    }

    free(chip8);
    destroyMultimediaLayer(mult);

    return 0;
}