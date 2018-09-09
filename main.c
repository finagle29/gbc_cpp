#include "gbz80.h"
#include "mmu.h"
#include "gpu.h"
#include "keys.h"
#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <signal.h>
#include <string.h>

bool frame(void);

key_type key;

_Noreturn void INThandler(int);

int main(int argc, char* argv[]) {

        char game_name[50];

        if (argc == 1) {
                strcpy(game_name, "cpu_instrs.gb");
        } else {
                strcpy(game_name, argv[1]);
        }


        key.rows[1] = 0xF;
        key.rows[2] = 0xF;
        key.column = 0;

        init_z80();
        // init_mmu();

        setup();

        signal(SIGINT, INThandler);
        
        load_rom(game_name);
        
        mmu->inbios = true;
        
        //load("DMG_ROM_friendly.bin", 0x0);
        load("DMG_ROM.bin", 0x0);
        
        // scramble_z80();
        mmu->inbios = true;
        
        bool quit = false;
        bool runframe = true;

        while (!getc(stdin)) {};


        SDL_Event event;
        while (!quit) {
                while (SDL_PollEvent(&event)) {
                        switch (event.type)
                        {
                                case SDL_QUIT:
                                        quit = true;
                                        break;
                                case SDL_KEYDOWN:
                                        switch (event.key.keysym.sym)
                                        {
                                                case SDLK_RETURN:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[1] &= 0x7;
                                                        break;
                                                case SDLK_DOWN:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[2] &= 0x7;
                                                        break;
                                                case SDLK_DELETE:
                                                case SDLK_BACKSPACE:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[1] &= 0xB;
                                                        break;
                                                case SDLK_UP:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[2] &= 0xB;
                                                        break;
                                                case SDLK_x:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[1] &= 0xD;
                                                        break;
                                                case SDLK_LEFT:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[2] &= 0xD;
                                                        break;
                                                case SDLK_z:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[1] &= 0xE;
                                                case SDLK_RIGHT:
                                                        z80.int_f |= 1 << 4;
                                                        key.rows[2] &= 0xE;
                                                        break;
                                        }
                                        break;
                                case SDL_KEYUP:
                                        switch (event.key.keysym.sym)
                                        {
                                                case SDLK_RETURN:
                                                        key.rows[1] |= 0x8;
                                                        break;
                                                case SDLK_DOWN:
                                                        key.rows[2] |= 0x8;
                                                        break;
                                                case SDLK_DELETE:
                                                case SDLK_BACKSPACE:
                                                        key.rows[1] |= 0x4;
                                                case SDLK_UP:
                                                        key.rows[2] |= 0x4;
                                                        break;
                                                case SDLK_x:
                                                        key.rows[1] |= 0x2;
                                                case SDLK_LEFT:
                                                        key.rows[2] |= 0x2;
                                                        break;
                                                case SDLK_z:
                                                        key.rows[1] |= 0x1;
                                                case SDLK_RIGHT:
                                                        key.rows[2] |= 0x1;
                                                        break;
                                        }
                                        break;
                        }
                }


                if (runframe) {
                       runframe = !frame();
                } else {
                        SDL_Delay(1000);
                }
        }

        cleanup();

        return 0;
}

bool frame() {
        int i = 0;
        bool quit = false;

        do {
                quit = fetch_dispatch_execute() ? true : false;
                i++;

                if (z80.pc == 0xC280) {
                        asm("nop");
                }
                
        } while (i < 20000 && !quit);
        return quit;
}

void INThandler(int sig) {
        printf("At 0x%04X,\n\tz80.af: 0x%04X\tz80.bc: 0x%04X\n", z80.pc, z80.af, z80.bc);
        printf("\tz80.de: 0x%04X\tz80.hl: 0x%04X\n", z80.de, z80.hl);
        printf("\tz80.if: 0x%02X\tz80.ie: 0x%02X\n", z80.int_f, z80.int_en);
        printf("\tz80.ime: %d\n", z80.ime);

        FILE *f = fopen("hram_dump.bin", "wb");
        fwrite(mmu->zram, 1, 0x80, f);
        fclose(f);

        exit(0);
}
