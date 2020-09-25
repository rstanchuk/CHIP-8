#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

struct chip8 {
    uint8_t keypad[16];
	uint32_t video[64 * 32];
    uint8_t memory[4096];
	uint8_t registers[16];
	uint16_t index;
	uint16_t pc;
	uint8_t delayTimer;
	uint8_t soundTimer;
	uint16_t stack[16];
	uint8_t sp;
	uint16_t opcode;
};

struct chip8* init();
void load(struct chip8*, const char*);
void cycle(struct chip8*);

struct MultimediaLayer {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
};

struct MultimediaLayer* makeMultimediaLayer(char const*, int, int, int, int);
void destroyMultimediaLayer(struct MultimediaLayer*);
bool processInput(struct MultimediaLayer*, uint8_t*);
void updateMultimediaLayer(struct MultimediaLayer*, void const*, int);

#endif