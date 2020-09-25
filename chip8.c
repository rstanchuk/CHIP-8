#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "chip8.h"

const unsigned int KEY_COUNT = 16;
const unsigned int MEMORY_SIZE = 4096;
const unsigned int REGISTER_COUNT = 16;
const unsigned int STACK_LEVELS = 16;
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;

uint8_t fontset[FONTSET_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

//OPCODES

//00E0 - CLS -- Clear the display.
void OP_00E0(struct chip8* chip) {
    memset(chip->video, 0, sizeof(chip->video));
}

//00EE - RET -- Return from a subroutine.
void OP_00EE(struct chip8* chip) {
    --chip->sp;
    chip->pc = chip->stack[chip->sp];
}

//1nnn - JP to addr nnn -- Jump to location nnn.
void OP_1nnn(struct chip8* chip) {
    uint16_t address = chip->opcode & 0x0FFF;

    chip->pc = address;
}

//2nnn - Call addr nnn -- Call subroutine at nnn.
void OP_2nnn(struct chip8* chip) {
    uint16_t address = chip->opcode & 0x0FFF;

    chip->stack[chip->sp] = chip->pc;
    ++chip->sp;
    chip->pc = address;
}

//3xkk - SE Vx, byte -- Skip next instruction if Vx = kk.
void OP_3xkk(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t byte = chip->opcode & 0x00FFu;

	if (chip->registers[Vx] == byte) {
		chip->pc += 2;
	}
}

//4xkk - SNE Vx, byte -- Skip next instruction if Vx != kk.
void OP_4xkk(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t byte = chip->opcode & 0x00FFu;

	if (chip->registers[Vx] != byte) {
		chip->pc += 2;
	}
}

//5xy0 - SE Vx, Vy -- Skip next instruction if Vx = Vy.
void OP_5xy0(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	if (chip->registers[Vx] == chip->registers[Vy]) {
		chip->pc += 2;
	}
}

//6xkk - LD Vx, byte -- Set Vx = kk.
void OP_6xkk(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t byte = chip->opcode & 0x00FFu;

	chip->registers[Vx] = byte;
}

//7xkk - ADD Vx, byte -- Set Vx = Vx + kk.
void OP_7xkk(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t byte = chip->opcode & 0x00FFu;

	chip->registers[Vx] += byte;
}

//8xy0 - LD Vx, Vy -- Set Vx = Vy.
void OP_8xy0(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	chip->registers[Vx] = chip->registers[Vy];
}

//8xy1 - OR Vx, Vy -- Set Vx = Vx OR Vy.
void OP_8xy1(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	chip->registers[Vx] |= chip->registers[Vy];
}

//8xy2 - AND Vx, Vy -- Set Vx = Vx AND Vy.
void OP_8xy2(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	chip->registers[Vx] &= chip->registers[Vy];
}

//8xy3 - XOR Vx, Vy -- Set Vx = Vx XOR Vy.
void OP_8xy3(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	chip->registers[Vx] ^= chip->registers[Vy];
}

//8xy4 - ADD Vx, Vy -- Set Vx = Vx + Vy, set VF = carry.
void OP_8xy4(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	uint16_t sum = chip->registers[Vx] + chip->registers[Vy];

	if (sum > 255U) {
		chip->registers[0xF] = 1;
	}
	else {
		chip->registers[0xF] = 0;
	}

	chip->registers[Vx] = sum & 0xFFu;
}

//8xy5 - SUB Vx, Vy -- Set Vx = Vx - Vy, set VF = NOT borrow.
void OP_8xy5(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	if (chip->registers[Vx] > chip->registers[Vy]) {
		chip->registers[0xF] = 1;
	}
	else {
		chip->registers[0xF] = 0;
	}

	chip->registers[Vx] -= chip->registers[Vy];
}

//8xy6 - SHR Vx -- Set Vx = Vx SHR 1.
void OP_8xy6(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	// Save LSB in VF
	chip->registers[0xF] = (chip->registers[Vx] & 0x1u);

	chip->registers[Vx] >>= 1;
}

//8xy7 - SUBN Vx, Vy -- Set Vx = Vy - Vx, set VF = NOT borrow.
void OP_8xy7(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	if (chip->registers[Vy] > chip->registers[Vx]) {
		chip->registers[0xF] = 1;
	}
	else {
		chip->registers[0xF] = 0;
	}

	chip->registers[Vx] = chip->registers[Vy] - chip->registers[Vx];
}

//8xyE - SHL Vx {, Vy} -- Set Vx = Vx SHL 1.
void OP_8xyE(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	// Save MSB in VF
	chip->registers[0xF] = (chip->registers[Vx] & 0x80u) >> 7u;

	chip->registers[Vx] <<= 1;
}

//9xy0 - SNE Vx, Vy -- Skip next instruction if Vx != Vy.
void OP_9xy0(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;

	if (chip->registers[Vx] != chip->registers[Vy]) {
		chip->pc += 2;
	}
}

//Annn - LD I, addr -- Set I = nnn.
void OP_Annn(struct chip8* chip) {
	uint16_t address = chip->opcode & 0x0FFFu;

	chip->index = address;
}

//Bnnn - JP V0, addr -- Jump to location nnn + V0.
void OP_Bnnn(struct chip8* chip) {
	uint16_t address = chip->opcode & 0x0FFFu;

	chip->pc = chip->registers[0] + address;
}

//Cxkk - RND Vx, byte -- Set Vx = random byte AND kk.
void OP_Cxkk(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t byte = chip->opcode & 0x00FFu;

	chip->registers[Vx] = rand() & byte;
}

//Dxyn - DRW Vx, Vy, nibble -- Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
void OP_Dxyn(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (chip->opcode & 0x00F0u) >> 4u;
	uint8_t height = chip->opcode & 0x000Fu;

	// Wrap if going beyond screen boundaries
	uint8_t xPos = chip->registers[Vx] % VIDEO_WIDTH;
	uint8_t yPos = chip->registers[Vy] % VIDEO_HEIGHT;

	chip->registers[0xF] = 0;

	for (unsigned int row = 0; row < height; ++row)
	{
		uint8_t spriteByte = chip->memory[chip->index + row];

		for (unsigned int col = 0; col < 8; ++col)
		{
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &(chip->video[(yPos + row) * VIDEO_WIDTH + (xPos + col)]);

			// Sprite pixel is on
			if (spritePixel)
			{
				// Screen pixel also on - collision
				if (*screenPixel == 0xFFFFFFFF)
				{
					chip->registers[0xF] = 1;
				}

				// Effectively XOR with the sprite pixel
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

//Ex9E - SKP Vx -- Skip next instruction if key with the value of Vx is pressed.
void OP_Ex9E(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	uint8_t key = chip->registers[Vx];

	if (chip->keypad[key]) {
		chip->pc += 2;
	}
}

//ExA1 - SKNP Vx -- Skip next instruction if key with the value of Vx is not pressed.
void OP_ExA1(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	uint8_t key = chip->registers[Vx];

	if (!chip->keypad[key]) {
		chip->pc += 2;
	}
}

//Fx07 - LD Vx, DT -- Set Vx = delay timer value.
void OP_Fx07(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	chip->registers[Vx] = chip->delayTimer;
}

//Fx0A - LD Vx, K -- Wait for a key press, store the value of the key in Vx.
void OP_Fx0A(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	if (chip->keypad[0]) {
		chip->registers[Vx] = 0;
	}
	else if(chip->keypad[1]) {
		chip->registers[Vx] = 1;
	}
	else if(chip->keypad[2]) {
		chip->registers[Vx] = 2;
	}
	else if(chip->keypad[3]) {
		chip->registers[Vx] = 3;
	}
	else if(chip->keypad[4]) {
		chip->registers[Vx] = 4;
	}
	else if(chip->keypad[5]) {
		chip->registers[Vx] = 5;
	}
	else if(chip->keypad[6]) {
		chip->registers[Vx] = 6;
	}
	else if(chip->keypad[7]) {
		chip->registers[Vx] = 7;
	}
    else if(chip->keypad[8]) {
		chip->registers[Vx] = 8;
	}
	else if(chip->keypad[9]) {
		chip->registers[Vx] = 9;
	}
	else if (chip->keypad[10]) {
		chip->registers[Vx] = 10;
	}
	else if(chip->keypad[11]) {
		chip->registers[Vx] = 11;
	}
	else if(chip->keypad[12]) {
		chip->registers[Vx] = 12;
	}
	else if(chip->keypad[13]) {
		chip->registers[Vx] = 13;
	}
	else if (chip->keypad[14]) {
		chip->registers[Vx] = 14;
	}
	else if(chip->keypad[15]) {
		chip->registers[Vx] = 15;
	}
	else {
		chip->pc -= 2;
	}
}

//Fx15 - LD DT, Vx -- Set delay timer = Vx.
void OP_Fx15(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	chip->delayTimer = chip->registers[Vx];
}

//Fx18 - LD ST, Vx -- Set sound timer = Vx.
void OP_Fx18(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	chip->soundTimer = chip->registers[Vx];
}

//Fx1E - ADD I, Vx -- Set I = I + Vx.
void OP_Fx1E(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	chip->index += chip->registers[Vx];
}

//Fx29 - LD F, Vx -- Set I = location of sprite for digit Vx.
void OP_Fx29(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t digit = chip->registers[Vx];

	chip->index = FONTSET_START_ADDRESS + (5 * digit);
}

//Fx33 - LD B, Vx -- Store BCD representation of Vx in memory locations I, I+1, and I+2.
void OP_Fx33(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;
	uint8_t value = chip->registers[Vx];

	// Ones-place
	chip->memory[chip->index + 2] = value % 10;
	value /= 10;

	// Tens-place
	chip->memory[chip->index + 1] = value % 10;
	value /= 10;

	// Hundreds-place
	chip->memory[chip->index] = value % 10;
}

//Fx55 - LD [I], Vx -- Store registers V0 through Vx in memory starting at location I.
void OP_Fx55(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i)
	{
		chip->memory[chip->index + i] = chip->registers[i];
	}
}

//Fx65 - LD Vx, [I] -- Read registers V0 through Vx from memory starting at location I.
void OP_Fx65(struct chip8* chip) {
	uint8_t Vx = (chip->opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i)
	{
		chip->registers[i] = chip->memory[chip->index + i];
	}
}

void execute_opcode(struct chip8* chip) {
    switch(chip->opcode & 0xF000) {
        //1nnn
        case 0x1000:
            OP_1nnn(chip);
            break;
        //2nnn
        case 0x2000:
            OP_2nnn(chip);
            break;
        //3xkk
        case 0x3000:
            OP_3xkk(chip);
            break;
        //4xkk
        case 0x4000:
            OP_4xkk(chip);
            break;
        //5xy0
        case 0x5000:
            OP_5xy0(chip);
            break;
        //6xkk
        case 0x6000:
            OP_6xkk(chip);
            break;
        //7xkk
        case 0x7000:
            OP_7xkk(chip);
            break;
        //9xy0
        case 0x9000:
            OP_9xy0(chip);
            break;
        //Annn
        case 0xA000:
            OP_Annn(chip);
            break;
        //Bnnn
        case 0xB000:
            OP_Bnnn(chip);
            break;
        //Cxkk
        case 0xC000:
            OP_Cxkk(chip);
            break;
        //Dxyn
        case 0xD000:
            OP_Dxyn(chip);
            break;
        //8xy_
        case 0x8000:
            switch(chip->opcode & 0x000F) {
                //8xy0
                case 0x0000:
                    OP_8xy0(chip);
                    break;
                //8xy1
                case 0x0001:
                    OP_8xy1(chip);
                    break;
                //8xy2
                case 0x0002:
                    OP_8xy2(chip);
                    break;
                //8xy3
                case 0x0003:
                    OP_8xy3(chip);
                    break;
                //8xy4
                case 0x0004:
                    OP_8xy4(chip);
                    break;
                //8xy5
                case 0x0005:
                    OP_8xy5(chip);
                    break;
                //8xy6
                case 0x0006:
                    OP_8xy6(chip);
                    break;
                //8xy7
                case 0x0007:
                    OP_8xy7(chip);
                    break;
                //8xyE
                case 0x000E:
                    OP_8xyE(chip);
                    break;
                default:
                    printf("Error: Unknown opcode.\n OPCODE - %X\n", chip->opcode);
                    exit(1);
            }
            break;
        //00E_
        case 0x0000:
            switch (chip->opcode & 0x000F) {
                //00E0
                case 0x0000:
                    OP_00E0(chip);
                    break;
                //00EE
                case 0x000E:
                    OP_00EE(chip);
                    break;
                default:
                    printf("Error: Unknown opcode.\n OPCODE - %X\n", chip->opcode);
                    exit(1);
            }
            break;
        //Ex__
        case 0xE000:
            switch(chip->opcode & 0x00FF) {
                //ExA1
                case 0x00A1:
                    OP_ExA1(chip);
                    break;
                //Ex9E
                case 0x009E:
                    OP_Ex9E(chip);
                    break;
                default:
                    printf("Error: Unknown opcode.\n OPCODE - %X\n", chip->opcode);
                    exit(1);
            }
            break;
        //Fx__
        case 0xF000:
            switch(chip->opcode & 0x00FF) {
                //Fx07
                case 0x0007:
                    OP_Fx07(chip);
                    break;
                //Fx0A
                case 0x000A:
                    OP_Fx0A(chip);
                    break;
                //Fx15
                case 0x0015:
                    OP_Fx15(chip);
                    break;
                //Fx18
                case 0x0018:
                    OP_Fx18(chip);
                    break;
                //Fx1E
                case 0x001E:
                    OP_Fx1E(chip);
                    break;
                //Fx29
                case 0x0029:
                    OP_Fx29(chip);
                    break;
                //Fx33
                case 0x0033:
                    OP_Fx33(chip);
                    break;
                //Fx55
                case 0x0055:
                    OP_Fx55(chip);
                    break;
                //Fx65
                case 0x0065:
                    OP_Fx65(chip);
                    break;
                default:
                    printf("Error: Unknown opcode.\n OPCODE - %X\n", chip->opcode);
                    exit(1);
            }
            break;
        default:
            printf("Error: Unknown opcode.\n OPCODE - %X\n", chip->opcode);
            exit(1);
    }
}

// CHIP-8 methods

struct chip8* init() {
    struct chip8* chip = (struct chip8*)malloc(sizeof(struct chip8));

    if(chip == NULL) {
        printf("Error: Wasn't able to initiate process.\n");
        exit(1);
    }

    chip->pc = START_ADDRESS;

    for (int i = 0; i < FONTSET_SIZE; ++i)
	{
		chip->memory[FONTSET_START_ADDRESS + i] = fontset[i];
	}

    srand(time(NULL));

    return chip;
}

void load(struct chip8* chip, const char* file_path) {
    FILE* rom = fopen(file_path, "rb");
    if (rom == NULL) {
        printf("Error: Failed to open ROM.\n");
        exit(1);
    }

    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    char* rom_buffer = (char*) malloc(sizeof(char) * rom_size);
    if (rom_buffer == NULL) {
        printf("Error: Failed to allocate memory for ROM.\n");
        exit(1);
    }

    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);
    if (result != rom_size) {
        printf("Error: Failed to read ROM.\n");
        exit(1);
    }

    if ((4096-512) > rom_size){
        for (int i = 0; i < rom_size; ++i) {
            chip->memory[i + START_ADDRESS] = (uint8_t)rom_buffer[i];
        }
    }
    else {
        printf("ROM too large to fit in memory.\n");
        exit(1);
    }

    fclose(rom);
    free(rom_buffer);
}

void cycle(struct chip8* chip) {
    chip->opcode = chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1];

    chip->pc += 2;

    execute_opcode(chip);   

    if(chip->delayTimer > 0) {
        --chip->delayTimer;
    }

    if(chip->soundTimer > 0) {
        if(chip->soundTimer == 1) {
            //Implement sound
        }
        --chip->soundTimer;
    }
}

// Multimedia methods

struct MultimediaLayer* makeMultimediaLayer(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight) {
    struct MultimediaLayer* mult = (struct MultimediaLayer*)malloc(sizeof(struct MultimediaLayer));

    if(mult == NULL) {
        printf("Error: Wasn't able to create multimedia layer.\n");
        exit(1);
    }

	SDL_Init(SDL_INIT_VIDEO);
	mult->window = SDL_CreateWindow(title, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
	mult->renderer = SDL_CreateRenderer(mult->window, -1, SDL_RENDERER_ACCELERATED);
	mult->texture = SDL_CreateTexture(mult->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

    return mult;
}

void destroyMultimediaLayer(struct MultimediaLayer* mult) {
    SDL_DestroyTexture(mult->texture);
	SDL_DestroyRenderer(mult->renderer);
	SDL_DestroyWindow(mult->window);
	SDL_Quit();

    free(mult);
}

bool processInput(struct MultimediaLayer* mult, uint8_t* keys) {
    bool run = true;

    SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				run = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
						run = false;
						break;
					case SDLK_1:
						keys[1] = 1;
						break;
					case SDLK_2:
						keys[2] = 1;
						break;
					case SDLK_3:
						keys[3] = 1;
						break;
					case SDLK_4:
						keys[0xC] = 1;
						break;
					case SDLK_q:
						keys[4] = 1;
						break;
					case SDLK_w:
						keys[5] = 1;
						break;
					case SDLK_e:
						keys[6] = 1;
						break;
					case SDLK_r:
						keys[0xD] = 1;
						break;
					case SDLK_a:
						keys[7] = 1;
						break;
					case SDLK_s:
						keys[8] = 1;
						break;
					case SDLK_d:
						keys[9] = 1;
						break;
					case SDLK_f:
						keys[0xE] = 1;
						break;
					case SDLK_z:
						keys[0xA] = 1;
						break;
					case SDLK_x:
						keys[0] = 1;
						break;
					case SDLK_c:
						keys[0xB] = 1;
						break;
					case SDLK_v:
						keys[0xF] = 1;
						break;
				}
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
						run = false;
						break;
					case SDLK_1:
						keys[1] = 0;
						break;
					case SDLK_2:
						keys[2] = 0;
						break;
					case SDLK_3:
						keys[3] = 0;
						break;
					case SDLK_4:
						keys[0xC] = 0;
						break;
					case SDLK_q:
						keys[4] = 0;
						break;
					case SDLK_w:
						keys[5] = 0;
						break;
					case SDLK_e:
						keys[6] = 0;
						break;
					case SDLK_r:
						keys[0xD] = 0;
						break;
					case SDLK_a:
						keys[7] = 0;
						break;
					case SDLK_s:
						keys[8] = 0;
						break;
					case SDLK_d:
						keys[9] = 0;
						break;
					case SDLK_f:
						keys[0xE] = 0;
						break;
					case SDLK_z:
						keys[0xA] = 0;
						break;
					case SDLK_x:
						keys[0] = 0;
						break;
					case SDLK_c:
						keys[0xB] = 0;
						break;
					case SDLK_v:
						keys[0xF] = 0;
						break;
				}
				break;
		}
	}

    return run;
}

void updateMultimediaLayer(struct MultimediaLayer* mult, void const* buffer, int pitch) {
	SDL_UpdateTexture(mult->texture, NULL, buffer, pitch);
	SDL_RenderClear(mult->renderer);
	SDL_RenderCopy(mult->renderer, mult->texture, NULL, NULL);
	SDL_RenderPresent(mult->renderer);
}
