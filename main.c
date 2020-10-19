#include "gbz80.h"
#include "mmu.h"
#include "apu.h"
#include "gpu.h"
#include "keys.h"
#include "gb_apu/MB_Wrapper.h"
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <signal.h>
#include <string.h>

bool frame(void);

key_type key;

_Noreturn void INThandler(int);

Multi_Buffer* buf; // Stereo_Buffer

SDL_AudioDeviceID dev;

void audio_callback(void* userdata, char* stream, int len) {
        Multi_Buffer_read_samples(buf, stream, len);
}

int main(int argc, char* argv[]) {

        char game_name[50];
        bool tileset, bgmap;
        int i;
        unsigned int lastTime = 0, currentTime;

        tileset = false;
        bgmap = false;
        game_name[0] = 0;

        // search through args for -d
        // then set game_name
        //
        for (i = 0; i < argc; i++) {
                if (strcmp(argv[i], "-d") == 0) {
                        tileset = true;
                        bgmap = true;
                }
                if (strstr(argv[i], ".gb") != NULL) {
                        strcpy(game_name, argv[i]);
                }
        }

        if (game_name[0] == 0) {
                strcpy(game_name, "cpu_instrs.gb");
        }


        key.rows[1] = 0xF;
        key.rows[2] = 0xF;
        key.column = 0;

        init_z80();
        // init_mmu();

        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        init_apu();
        setup(tileset, bgmap);
        atexit(SDL_Quit);

        buf = new_Stereo_Buffer();
        blargg_err_t error = Multi_Buffer_set_sample_rate_msec(buf, 44100, 1000);
        if (error)
                fprintf(stderr, "%s", error);
        Multi_Buffer_clock_rate(buf, 4194304);

        Gb_Apu_output_3(apu, Stereo_Buffer_center(buf),
                        Stereo_Buffer_left(buf),
                        Stereo_Buffer_right(buf));

        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 2048;
        want.callback = audio_callback;

        dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

        SDL_PauseAudioDevice(dev, 0);


        signal(SIGINT, INThandler);
        
        load_rom(game_name);
        
        mmu->inbios = true;
        
        //load("DMG_ROM_friendly.bin", 0x0);
        load_bios("DMG_ROM.bin");
        
        // scramble_z80();
        mmu->inbios = true;
        
        bool quit = false;
        bool runframe = true;

        // while (!getc(stdin)) {};


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
                                                        break;
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
                                                        break;
                                                case SDLK_UP:
                                                        key.rows[2] |= 0x4;
                                                        break;
                                                case SDLK_x:
                                                        key.rows[1] |= 0x2;
                                                        break;
                                                case SDLK_LEFT:
                                                        key.rows[2] |= 0x2;
                                                        break;
                                                case SDLK_z:
                                                        key.rows[1] |= 0x1;
                                                        break;
                                                case SDLK_RIGHT:
                                                        key.rows[2] |= 0x1;
                                                        break;
                                        }
                                        break;
                        }
                }


                if (runframe) {
                        lastTime = SDL_GetTicks();
                        runframe = !frame();
                        currentTime = SDL_GetTicks();
                        lastTime = (0 < (int)(17 + lastTime - currentTime)) ?
                                (17 + lastTime - currentTime) : 0;
                        SDL_Delay(lastTime);
                } else {
                        SDL_Delay(1000);
                }
        }

        cleanup();

        return 0;
}

bool frame() {
        int i = 0;
        int cycles = 0;
        bool quit = false;
        bool stereo;
        size_t count;

        do {
                cycles = fetch_dispatch_execute();
                quit = cycles ? false : true;
                i += cycles;



                if (z80.pc == 0xC280) {
                        __asm__("nop");
                }
                
        } while (i < 17564 && !quit);
        z80.clock.m = 0;

        stereo = Gb_Apu_end_frame(apu, i*4);
        Multi_Buffer_end_frame(buf, i*4, stereo);

        return quit;
}

void INThandler(int sig) {
        printf("At 0x%04X,\n\tz80.af: 0x%04X\tz80.bc: 0x%04X\n", z80.pc, z80.af, z80.bc);
        printf("\tz80.de: 0x%04X\tz80.hl: 0x%04X\n", z80.de, z80.hl);
        printf("\tz80.if: 0x%02X\tz80.ie: 0x%02X\n", z80.int_f, z80.int_en);
        printf("\tz80.ime: %d\n", z80.ime);
        printf("\n\n");
        printf("LCDC: 0x%02X\tSTAT: 0x%02X\n", gpu.gpu_ctrl, gpu.gpu_stat);

        int c;
        do {
                printf("\nDo you really want to quit? (y/n) ");
                c = getc(stdin);
        } while (!(c == 'y' || c == 'Y' || c == 'n' || c == 'N'));

        if (c == 'n' || c == 'N') {
                return;
        }

        FILE *f = fopen("hram_dump.bin", "wb");
        fwrite(mmu->zram, 1, 0x80, f);
        fclose(f);

        f = fopen("oam_dump.bin", "wb");
        fwrite(gpu.oam, 1, 0xA0, f);
        fclose(f);
        cleanup();

        exit(0);
}
