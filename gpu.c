#include <SDL.h>
#include <stdio.h>
#include <assert.h>
#include "gpu.h"
#include "gbz80.h"
#include "mmu.h"

gpu_type gpu;

static SDL_Window *window, *vram_w, *bg_w;
static SDL_Renderer *renderer, *vram_r, *bg_r;
// static SDL_Texture* framebuffer;
// static unsigned int *pixels;

#define RGBA(r, g, b) (((((r << 8) | g) << 8) | b) << 8) | 0xFF

static bool show_tileset, show_bgmap;

static unsigned char dmg_pal_a[4][3] = {
        {156, 189, 15},
        {140, 173, 15},
        {48, 98, 48},
        {15, 56, 15}
};

static unsigned int dmg_pal[4] = {
        RGBA(156, 189, 15),
        RGBA(140, 173, 15),
        RGBA(48, 98, 48),
        RGBA(15, 56, 15)
};

static unsigned char bgb_pal_a[4][3] = {
        {224, 248, 208},
        {136, 192, 112},
        {52, 104, 86},
        {8, 24, 32}
};

static unsigned int bgb_pal[4] = {
        RGBA(224, 248, 208),
        RGBA(136, 192, 112),
        RGBA(52, 104, 86),
        RGBA(8, 24, 32)
};

static unsigned char bw_pal_a[4][3] = {
        {255, 255, 255},
        {170, 170, 170},
        {85, 85, 85},
        {0, 0, 0}
};

static unsigned int bw_pal[4] = {
        RGBA(255, 255, 255),
        RGBA(170, 170, 170),
        RGBA(85, 85, 85),
        RGBA(0, 0, 0)
};

static unsigned char mo_pal_a[4][3] = {
        {255, 255, 176},
        {255, 195, 0},
        {234, 126, 11},
        {13, 79, 32}
};

static unsigned int mo_pal[4] = {
        RGBA(255, 255, 176),
        RGBA(255, 195, 0),
        RGBA(234, 126, 11),
        RGBA(13, 79, 32)
};

unsigned int pixels[160*144];

#define BGB

#if defined(DMG)
        #define PAL dmg_pal
        #define PAL_A dmg_pal_a
#elif defined(BGB)
        #define PAL bgb_pal
        #define PAL_A bgb_pal_a
#elif defined(MO)
        #define PAL mo_pal
        #define PAL_A mo_pal_a
#else
        #define PAL bw_pal
        #define PAL_A bw_pal_a
#endif

void dump_vram() {
        FILE *f = fopen("vram_dump.bin", "wb");

        if (f == NULL) {
                fprintf(stderr, "Error opening vram dump file");
                return;
        }

        fwrite(gpu.vram, sizeof(unsigned char), 0x2000, f);
        fclose(f);
}

void fetcher_tick() {
        unsigned short mapoffs;
        unsigned char lineoffs, upper, lower;
        switch (gpu.fetcher.mode) {
                case 0:
                        // read tile #
                        mapoffs = GPU_BG_MAP ? 0x1C00 : 0x1800;
                        lineoffs = ((gpu.scrollX + gpu.xpos) >> 3) & 0x1F;
                        if (gpu.line > 144) {
                                return;
                        }
                        mapoffs += (((gpu.line + gpu.scrollY) & 0xFF) >> 3) << 5;
                        gpu.fetcher.tile = (unsigned short)gpu.vram[mapoffs + lineoffs];
                        if (!GPU_BG_SET && gpu.fetcher.tile < 128) gpu.fetcher.tile += 256;
                        gpu.fetcher.mode++;
                        // printf("fetcher tick read tile\n");
                        break;
                case 1:
                        gpu.fetcher.mode++;
                        break;

                case 2:
                // read byte 1 of tile
                        lower = gpu.vram[gpu.fetcher.tile * 16 + 2 * ((gpu.line + gpu.scrollY) & 7)];
                        for (int i = 0; i < 8; i++) {
                                gpu.fetcher.pixels[7-i] = GET_BIT(lower, i);
                                // gpu.fetcher.pixels[i] = 1 & gpu.tileset[gpu.fetcher.tile][7 & (gpu.line + gpu.scrollY)][i];
                        }
                        gpu.fetcher.mode++;
                        // printf("fetcher tick read byte 1\n");
                        break;
                case 3:
                        gpu.fetcher.mode++;
                        break;
                case 4:
                // read byte 2 of tile
                        upper = gpu.vram[gpu.fetcher.tile * 16 + 2 * ((gpu.line + gpu.scrollY) & 7) + 1];
                        for (int i = 0; i < 8; i++) {
                                gpu.fetcher.pixels[7-i] |= GET_BIT(upper, i) << 1;
                                // gpu.fetcher.pixels[i] |= 2 & gpu.tileset[gpu.fetcher.tile][7 & (gpu.line + gpu.scrollY)][i];
                        }
                        gpu.fetcher.mode++;
                        gpu.fetcher.ready = true;
                        // printf("fetcher tick read byte 2\n");
                        break;
                case 5:
                        gpu.fetcher.mode++;
                        break;
                case 6:
                        gpu.fetcher.mode++;
                        break;
                case 7:
                // idle
                        gpu.fetcher.mode = 0;
                        // printf("fetcher tick idle\n");
                        break;

        }
        if (gpu.fetcher.ready && (gpu.bg_FIFO.size <= 8)) {
                memcpy(gpu.bg_FIFO.FIFO + gpu.bg_FIFO.end, gpu.fetcher.pixels, 8);
                gpu.bg_FIFO.size += 8;
                gpu.bg_FIFO.end = (gpu.bg_FIFO.end + 8) % 16;
                gpu.fetcher.ready = false;
                // printf("fetcher tick push to FIFO\n");
        }
}

void gpu_tick() {
        // consume 1 T-time in the GPU
        // ignore DMA, since that is done in M-time
        if (!GPU_DISP) {
                gpu.mode = 0;
                gpu.gpu_stat = gpu.mode |
                        ((gpu.line == gpu.lineYC) ? 1 : 0 << 2) |
                        (gpu.gpu_stat & 0xF8);
                return;
        }

        switch (gpu.mode)
        {
                // OAM read mode, scanline is active
                // actually read OAM here
                case 2:
                        // find up to 10 visible sprites
                        gpu.mode_clock++;
                        if (gpu.mode_clock >= 80) {
                                gpu.mode_clock = 0;
                                gpu.mode = 3;
                        }
                        break;
                // VRAM read mode, scanline active
                // treat end of mode 3 as end of scanline
                // push pixels from FIFO to framebuffer
                case 3:
                        // discard gpu.scrollX pixels from FIFO
                        // need to check that FIFO always has more than 8 pixels in it!!
                        // ONLY DO THIS AT BEGINNING OF LINE!!!!!
                        if (gpu.mode_clock < 16) {
                                fetcher_tick();
                                gpu.xpos++;
                                gpu.mode_clock++;
                                break;
                        }
                        if (gpu.mode_clock == 16) {
                                gpu.xpos = 0;
                        }
                        if (gpu.mode_clock < 16 + (gpu.scrollX & 8)) {
                                // FIFO_pop(&gpu.sprite_FIFO);
                                FIFO_pop(&gpu.bg_FIFO);
                                fetcher_tick();
                                gpu.mode_clock++;
                                break;
                        }

                        // at each step, check for window!!

                                // push 2 pixels
                                push_pixel();
                                fetcher_tick();
                                gpu.xpos++;
                                gpu.mode_clock++;

                                // fetcher reads tile #

                                // push 2 pixles
                                // fetcher reads 1 byte of data from VRAM
                                // push 2 pixels
                                // fetcher reads 1 byte of data from VRAM
                                // push 2 pixels
                                // load FIFO from fetcher
                        
                        // if window
                                // clear FIFO
                                // fetcher reads tile #
                                // fetcher reads 1 byte of data from VRAM
                                // fetcher reads 1 byte of data from VRAM
                                // load FIFO from fetcher

                        if (gpu.xpos >= 160) {
                                // enter HBlank
                                gpu.mode_clock = 0;
                                gpu.mode = 0;
                                gpu.xpos = 0;
                                
                                gpu.fetcher.mode = 0;
                                gpu.fetcher.xoffs = 0;
                                gpu.fetcher.ready = false;

                                gpu.bg_FIFO.size = 0;
                                gpu.bg_FIFO.start = 0;
                                gpu.bg_FIFO.end = 0;

                                // Write a scanline to the framebuffer
                                // renderscan();
                        }
                        break;

                // HBlank. after the last one push the screen data
                case 0:
                        gpu.mode_clock++;
                        if (gpu.mode_clock >= 204) {
                                gpu.mode_clock = 0;
                                gpu.line++;
                                // printf("\n");

                                if (gpu.line >= 143) {
                                        // enter VBlank
                                        gpu.mode = 1;
                                        z80.int_f |= 1;
                                        
                                        // renderscan();
                                        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
                                        SDL_UpdateTexture(texture, NULL, pixels, 160 * sizeof(unsigned int));
                                        
                                        SDL_RenderCopy(renderer, texture, NULL, NULL);
                                        SDL_DestroyTexture(texture);
                                        SDL_RenderPresent(renderer);
                                        if (show_tileset) showTileSet();
                                        if (show_bgmap) showBGMap();
                                        // printf("\n");
                                        /* put image data on the window */
                                } else {
                                        gpu.mode = 2;
                                }
                        }
                        break;

                // VBlank
                case 1:
                        gpu.mode_clock++;
                        if (gpu.mode_clock >= 456) {
                                gpu.line++;

                                if (gpu.line > 153) {
                                        // restart scanning mode
                                        gpu.mode = 2;
                                        gpu.line = 0;
                                        gpu.mode_clock = 0;
                                        gpu.bg_FIFO.size = 0;
                                        // printf("\n\n");
                                }
                        }
                        break;
        }
        if (((gpu.line == gpu.lineYC) && (gpu.gpu_stat & 0x40)) ||
                        ((gpu.mode == 0) && (gpu.gpu_stat & 0x8)) ||
                        ((gpu.mode == 2) && (gpu.gpu_stat & 0x20)) ||
                        ((gpu.mode == 1) && (gpu.gpu_stat & 0x30))) {
                z80_p->int_f |= 0x2;
        }
        gpu.gpu_stat = gpu.mode | ((gpu.line == gpu.lineYC) ? 1 : 0 << 2) |
                (gpu.gpu_stat & 0xF8);
}

void gpu_step() {
        unsigned char i;
        unsigned char time_to_consume = z80.t;

        if (gpu.do_DMA) {
                for (i = 0; i < z80.m; i++) {
                        gpu.oam[gpu.DMA_ptr] = rb((unsigned short)(gpu.DMA << 8) + gpu.DMA_ptr);
                        gpu.DMA_ptr++;
                        if (gpu.DMA_ptr >= 0xA0) {
                                gpu.do_DMA = false;
                                gpu.DMA_ptr = 0;
                                break;
                        }
                }
        }

        for (int t = 0; t < z80.t; t++) {
                gpu_tick();
        }
        return;

}

void fetch_pixels_FIFO() {
        // allowed to use z80.t cycles
        // do I need an internal state?
        return;
}

void renderscan() {
        /* Obtain pixels */
        unsigned short mapoffs = GPU_BG_MAP ? 0x1C00 : 0x1800;
        unsigned char lineoffs = gpu.scrollX >> 3;
        unsigned char y = (gpu.line + gpu.scrollY) & 7;
        unsigned char x = gpu.scrollX & 7;
        unsigned char color, i, sprite_num, sprite_flags, tile_x, tile_y, sprite_size;
        unsigned short tile;
        unsigned char scanline_row[160];

        if (gpu.line > 144) {
                return;
        }


        mapoffs += (((gpu.line + gpu.scrollY) & 0xFF) >> 3) << 5;
        tile = (unsigned short)gpu.vram[mapoffs + lineoffs];

        if (!GPU_BG_SET && tile < 128) tile += 256;
        if (GPU_BG) {
                for (i = 0; i < 160; i++) {
                        color = gpu.bg_pal >> (gpu.tileset[tile][y][x] * 2);
                        color &= 3;

                        scanline_row[i] = gpu.tileset[tile][y][x];

                        pixels[i + (gpu.line)*160] = PAL[color];

                        x++;
                        if (x == 8) {
                                x = 0;
                                lineoffs = (lineoffs + 1) & 31;
                                // if (gpu.line % 8 == 0) printf("%03d", tile);
                                tile = gpu.vram[mapoffs + lineoffs];
                                if (!GPU_BG_SET && tile < 128) tile += 256;
                        }
                }
        }
        // if (gpu.line % 8 == 0) printf("\n");
        if (GPU_WDOW) {
                mapoffs = GPU_WDOW_MAP ? 0x1C00 : 0x1800;
                lineoffs = 0;
                y = (gpu.line + gpu.wdow_y) & 7;
                x = 0;

                mapoffs += (((gpu.line - gpu.wdow_y) & 0xFF) >> 3) << 5;

                tile = (unsigned short)gpu.vram[mapoffs + lineoffs];

                if (!GPU_BG_SET && tile < 128) tile += 256;
                if (gpu.wdow_y <= gpu.line) {
                        for (i = gpu.wdow_x - 7; i < 160; i++) {
                                color = gpu.bg_pal >> (gpu.tileset[tile][y][x] * 2);
                                color &= 3;

                                scanline_row[i] = gpu.tileset[tile][y][x];

                                pixels[i + (gpu.line)*160] = PAL[color];

                                x++;
                                if (x == 8) {
                                        x = 0;
                                        lineoffs = (lineoffs + 1) & 31;
                                        // if (gpu.line % 8 == 0) printf("%03d", tile);
                                        tile = gpu.vram[mapoffs + lineoffs];
                                        if (!GPU_BG_SET && tile < 128) tile += 256;
                                }
                        }
                }
        }

        if (GPU_SPR) {
                sprite_size = 1 + GPU_SP_SZ;
                for (i = 0; i < 40; i++) {
                        y = gpu.oam[4 * i] - 16;
                        x = gpu.oam[4 * i + 1] - 8;
                        sprite_num = gpu.oam[4 * i + 2];
                        sprite_flags = gpu.oam[4 * i + 3];

                        unsigned char pal;

                        if (GET_BIT(sprite_flags, 4)) {
                                pal = gpu.ob_pal1;
                        } else {
                                pal = gpu.ob_pal0;
                        }

                        if (y <= gpu.line && (y + sprite_size*8) > gpu.line) {
                                if (GET_BIT(sprite_flags, 6)) {
                                        tile_y = 7 - (gpu.line - y)/sprite_size;
                                } else {
                                        tile_y = (gpu.line - y)/sprite_size;
                                }


                                for (tile_x = 0; tile_x < 8; tile_x++) {
                                        if ((x + tile_x) >= 0 &&
                                                        (x + tile_x) < 160 &&
                                                        (!GET_BIT(sprite_flags, 7) ||
                                                         !scanline_row[x + tile_x])) {
                                                if (GET_BIT(sprite_flags, 5)) {
                                                        color = gpu.tileset[sprite_num][tile_y][7 - tile_x];
                                                } else {
                                                        color = gpu.tileset[sprite_num][tile_y][tile_x];
                                                }

                                                if (color) {
                                                        //printf("sprite %d\n", i);
                                                        color = pal >> (color * 2);
                                                        color &= 3;

                                                        pixels[x + tile_x + (gpu.line)*160] = PAL[color];
                                                }
                                        }
                                }
                        }
                }
        }
}

void showBGMap() {
        unsigned char x, y, color;
        unsigned short tile, tile_addr;
        unsigned short mapoffs = GPU_BG_MAP ? 0x1C00 : 0x1800;

        unsigned int pixels_bg[256*256];

        for (tile = 0; tile < 1024; tile++) {
                tile_addr = gpu.vram[mapoffs + tile];
                if (!GPU_BG_SET && tile_addr < 128) tile_addr += 256;

                for (x = 0; x < 8; x++) {
                        for (y = 0; y < 8; y++) {
                                color = gpu.bg_pal >> (gpu.tileset[tile_addr][y][x] * 2);
                                color &= 3;

                                pixels_bg[((tile / 32)*8 + y) * 256 + (tile % 32)*8 + x] = PAL[color];

                        }
                }
        }
        SDL_Texture* texture = SDL_CreateTexture(bg_r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 256, 256);
        SDL_UpdateTexture(texture, NULL, pixels_bg, 256 * sizeof(unsigned int));
        SDL_RenderCopy(bg_r, texture, NULL, NULL);
        SDL_DestroyTexture(texture);

        SDL_Rect screen1 = {gpu.scrollX, gpu.scrollY, 160, 144};
        SDL_Rect screen2 = {gpu.scrollX, gpu.scrollY, 160, 144};
        SDL_Rect screen3 = {gpu.scrollX, gpu.scrollY, 160, 144};
        SDL_Rect screen4 = {gpu.scrollX, gpu.scrollY, 160, 144};
        
        if (screen1.x + screen1.w > 256) screen2.x -= 256;
        if (screen1.y + screen1.h > 256) screen3.y -= 256;
        if (screen1.x + screen1.w > 256 && screen1.y + screen1.h > 256) screen4.x -= 256; screen4.y -= 256;
        SDL_SetRenderDrawColor(bg_r, 255, 0, 0, 255);
        SDL_RenderDrawRect(bg_r, &screen1);
        SDL_RenderDrawRect(bg_r, &screen2);
        SDL_RenderDrawRect(bg_r, &screen3);
        SDL_RenderDrawRect(bg_r, &screen4);
        SDL_RenderPresent(bg_r);
}



void showTileSet() {
        unsigned char x, y, color;
        unsigned short tile;

        unsigned int pixels_vram[17*16*17*24];
        memset(pixels_vram, 0xFF, 17*16*24*17*sizeof(unsigned int));

        for (tile = 0; tile < 384; tile++) {
                for (x = 0; x < 8; x++) {
                        for (y = 0; y < 8; y++) {
                                color = gpu.bg_pal >> (gpu.tileset[tile][y][x] * 2);
                                color &= 3;

                                pixels_vram[((tile / 16)*17 + 2*y)*17*16 + (tile % 16)*17 + 2*x] = PAL[color];
                                pixels_vram[((tile / 16)*17 + 2*y)*17*16 + (tile % 16)*17 + 2*x + 1] = PAL[color];
                                pixels_vram[((tile / 16)*17 + 2*y + 1)*17*16 + (tile % 16)*17 + 2*x] = PAL[color];
                                pixels_vram[((tile / 16)*17 + 2*y + 1)*17*16 + (tile % 16)*17 + 2*x + 1] = PAL[color];

                        }
                }
        }
        SDL_Texture* texture = SDL_CreateTexture(vram_r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 17*16, 17*24);
        SDL_UpdateTexture(texture, NULL, pixels_vram, 17*16 * sizeof(unsigned int));
        SDL_RenderCopy(vram_r, texture, NULL, NULL);
        SDL_DestroyTexture(texture);
        SDL_RenderPresent(vram_r);
}


void setup(bool tileset, bool bgmap) {
        gpu.gpu_ctrl = 0;
        int i, x, y;
        SDL_Point window_size = {160, 144};
        
        show_tileset = tileset;
        show_bgmap = bgmap;


        SDL_CreateWindowAndRenderer(
                        window_size.x*2, window_size.y*2,
                        SDL_WINDOW_OPENGL, &window, &renderer);
        SDL_RenderSetLogicalSize(renderer, 160, 144);
        SDL_GetWindowPosition(window, &x, &y);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);


        if (tileset) {
                SDL_CreateWindowAndRenderer(
                                17*16, 17*24, 0, &vram_w, &vram_r);
                SDL_SetWindowPosition(vram_w, x-17*16, y);
                SDL_SetRenderDrawColor(vram_r, 255, 253, 208, 255);
                SDL_RenderClear(vram_r);
                SDL_RenderPresent(vram_r);
        }
        
        if (bgmap) {
                SDL_CreateWindowAndRenderer(
                                256, 256, 0,
                                &bg_w, &bg_r);
                SDL_SetWindowPosition(bg_w, x+160*2, y);
                SDL_SetRenderDrawColor(bg_r, 255, 253, 208, 255);
                SDL_RenderClear(bg_r);
                SDL_RenderPresent(bg_r);
        }


        for (i = 0; i < 1000; i++) {
                SDL_Delay(1);
                SDL_PumpEvents();
        }

}

void cleanup() {
        SDL_DestroyWindow(window);
        SDL_DestroyWindow(vram_w);
        SDL_DestroyWindow(bg_w);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyRenderer(vram_r);
        SDL_DestroyRenderer(bg_r);
        SDL_Quit();
}

unsigned char FIFO_pop(FIFO_t* fifo) {
        assert(fifo->size > 0);
        unsigned char val = fifo->FIFO[fifo->start];
        fifo->start = (fifo->start + 1) % 16;
        fifo->size--;
        return val;
}

void push_pixel() {
        unsigned char bg_pix, sprite_pix, color;
        bg_pix = FIFO_pop(&gpu.bg_FIFO);
        // sprite_pix = FIFO_pop(&gpu.sprite_FIFO);
        // for rn, ignore sprite_pix
        color = gpu.bg_pal >> (bg_pix * 2);
        color &= 3;
        // printf("%d", color);
        pixels[(gpu.line * 160) + gpu.xpos] = PAL[color];
}
