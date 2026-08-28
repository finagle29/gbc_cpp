#include <stdio.h>
#include "gpu.h"
#include "gbz80.h"
#include "mmu.h"

gpu_type gpu;

SDL_Window *window, *vram_w, *bg_w;
SDL_Renderer *renderer, *vram_r, *bg_r;
SDL_Texture *framebuffer, *framebuffer_tileset, *framebuffer_bgmap;

#define RGBA(r, g, b) (((((r << 8) | g) << 8) | b) << 8) | 0xFF

bool show_tileset, show_bgmap;

static unsigned char dmg_pal_a[4][3] = {
    {156, 189, 15},
    {140, 173, 15},
    {48, 98, 48},
    {15, 56, 15}};

static unsigned int dmg_pal[4] = {
    RGBA(156, 189, 15),
    RGBA(140, 173, 15),
    RGBA(48, 98, 48),
    RGBA(15, 56, 15)};

static unsigned char bgb_pal_a[4][3] = {
    {224, 248, 208},
    {136, 192, 112},
    {52, 104, 86},
    {8, 24, 32}};

static unsigned int bgb_pal[4] = {
    RGBA(224, 248, 208),
    RGBA(136, 192, 112),
    RGBA(52, 104, 86),
    RGBA(8, 24, 32)};

static unsigned char bw_pal_a[4][3] = {
    {255, 255, 255},
    {170, 170, 170},
    {85, 85, 85},
    {0, 0, 0}};

static unsigned int bw_pal[4] = {
    RGBA(255, 255, 255),
    RGBA(170, 170, 170),
    RGBA(85, 85, 85),
    RGBA(0, 0, 0)};

static unsigned char mo_pal_a[4][3] = {
    {255, 255, 176},
    {255, 195, 0},
    {234, 126, 11},
    {13, 79, 32}};

static unsigned int mo_pal[4] = {
    RGBA(255, 255, 176),
    RGBA(255, 195, 0),
    RGBA(234, 126, 11),
    RGBA(13, 79, 32)};

static unsigned int pixels[160 * 144];
static unsigned int prev_pixels[160 * 144];
static unsigned int tmp[160 * 144];
// unsigned int pixels_vram[17 * 16 * 17 * 24];
static unsigned int pixels_vram[(9 * 16 - 1) * (9 * 24 - 1)];
static unsigned int pixels_bg[256 * 256];

// #define DMG

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

void dump_vram()
{
        FILE *f = fopen("vram_dump.bin", "wb");

        if (f == NULL)
        {
                fprintf(stderr, "Error opening vram dump file");
                return;
        }

        fwrite(gpu.vram, sizeof(unsigned char), 0x2000, f);
        fclose(f);
}

void scan_oam()
{
        unsigned char color, i, j, x, y;
        unsigned char sprite_size, n_sprites, lowest_x, lowest_ix;
        unsigned char tmp_sprites[10];
        // read objects from OAM
        sprite_size = 1 + GPU_SP_SZ;
        n_sprites = 0;
        for (i = 0; (i < 40) & (n_sprites < 10); i++)
        {
                y = gpu.oam[4 * i] - 16;

                if (y <= gpu.line && (y + sprite_size * 8) > gpu.line)
                {
                        tmp_sprites[n_sprites] = i;
                        n_sprites++;
                        // save this sprite
                }
        }
        // objects are in OAM order
        // sort sprites first by x-coordinate then by OAM order
        for (i = 0; i < n_sprites; i++)
        {
                // search through sprites for lowest x coord
                lowest_x = gpu.oam[4 * tmp_sprites[i] + 1];
                lowest_ix = i;
                for (j = i; j < n_sprites; j++)
                {
                        x = gpu.oam[4 * tmp_sprites[j] + 1];
                        if (x < lowest_x)
                        {
                                lowest_x = x;
                                lowest_ix = j;
                        }
                }
                gpu.sprites[i] = tmp_sprites[lowest_ix];
                tmp_sprites[lowest_ix] = tmp_sprites[i];
        }
        gpu.num_sprites = i;
}

void gpu_m_tick()
{
        if (gpu.do_DMA)
        {
                if (gpu.DMA >= 0xE0)
                {
                        gpu.oam[gpu.DMA_ptr] = rb((unsigned short)((gpu.DMA - 0x20) << 8) + gpu.DMA_ptr);
                }
                else
                {
                        gpu.oam[gpu.DMA_ptr] = rb((unsigned short)(gpu.DMA << 8) + gpu.DMA_ptr);
                }
                gpu.DMA_ptr++;
                if (gpu.DMA_ptr >= 0xA0)
                {
                        gpu.do_DMA = false;
                        gpu.DMA_ptr = 0;
                }
        }
        if (gpu.reset_DMA)
        {
                gpu.do_DMA = true;
                gpu.reset_DMA = false;
                gpu.DMA_ptr = 0;
        }
        if (gpu.DMA_requested)
        {
                gpu.reset_DMA = true;
                gpu.DMA_requested = false;
                // no-op the first cycle
        }

        if (!GPU_DISP)
        {
                gpu.mode = 0;
                gpu.mode_clock = 0;
                gpu.line = 0;
                gpu.gpu_stat &= 0xFE;
                return;
        }
        gpu.mode_clock += 4;

        switch (gpu.mode)
        {
        // OAM read mode, scanline is active
        case 2:
                if (gpu.mode_clock >= 80)
                {
                        scan_oam();
                        gpu.mode_clock = 0;
                        gpu.mode = 3;
                }
                break;

        // VRAM read mode, scanline active
        // treat end of mode 3 as end of scanline
        case 3:
                if (gpu.mode_clock >= 172)
                {
                        // enter HBlank
                        gpu.mode_clock = 0;
                        if (
                            !(((gpu.gpu_stat & 0x40) && (gpu.gpu_stat & 0x4)) |
                              ((gpu.gpu_stat & 0x20) && (gpu.mode == 2)) |
                              ((gpu.gpu_stat & 0x10) && (gpu.mode == 1)) |
                              ((gpu.gpu_stat & 0x8) && (gpu.mode == 0))) // GPU STAT line is low
                            &&
                            (gpu.gpu_stat & 0x8))
                        {
                                z80_p->int_f |= 0x2;
                        };
                        gpu.mode = 0;

                        // Write a scanline to the framebuffer
                        renderscan();
                }
                break;

        // HBlank. after the last one push the screen data
        case 0:
                if (gpu.mode_clock >= 204)
                {
                        gpu.mode_clock = 0;
                        gpu.line++;

                        if (
                            !(((gpu.gpu_stat & 0x40) && (gpu.gpu_stat & 0x4)) |
                              ((gpu.gpu_stat & 0x20) && (gpu.mode == 2)) |
                              ((gpu.gpu_stat & 0x10) && (gpu.mode == 1)) |
                              ((gpu.gpu_stat & 0x8) && (gpu.mode == 0))) // GPU STAT line is low
                            &&
                            ((gpu.line == gpu.lineYC) && (gpu.gpu_stat & 0x40)))
                        {
                                z80_p->int_f |= 0x2;
                        };
                        gpu.gpu_stat |= (((gpu.line == gpu.lineYC) ? 1 : 0) << 2);

                        if (gpu.line == gpu.wdow_y)
                                gpu.window_YC = true;

                        if (gpu.line == 144)
                        {
                                // enter VBlank
                                if (
                                    !(((gpu.gpu_stat & 0x40) && (gpu.gpu_stat & 0x4)) |
                                      ((gpu.gpu_stat & 0x20) && (gpu.mode == 2)) |
                                      ((gpu.gpu_stat & 0x10) && (gpu.mode == 1)) |
                                      ((gpu.gpu_stat & 0x8) && (gpu.mode == 0))) // GPU STAT line is low
                                    &&
                                    ((gpu.gpu_stat & 0x10) | (gpu.gpu_stat & 0x20)))
                                {
                                        z80_p->int_f |= 0x2;
                                };
                                gpu.mode = 1;
                                z80.int_f |= 1;

                                for (int i = 0; i < 160 * 144; i++)
                                {
                                        tmp[i] = (pixels[i] / 2 + prev_pixels[i] / 2);
                                        prev_pixels[i] = pixels[i];
                                        tmp[i] = pixels[i];
                                }

                                renderscan();
                                // SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
                                SDL_UpdateTexture(framebuffer, NULL, tmp, 160 * sizeof(unsigned int));
                                // SDL_UpdateTexture(framebuffer, NULL, pixels, 160 * sizeof(unsigned int));

                                SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
                                // SDL_DestroyTexture(texture);
                                SDL_RenderPresent(renderer);

                                // framerate calculations go here
                                if (show_tileset)
                                        showTileSet();
                                if (show_bgmap)
                                        showBGMap();
                        }
                        else
                        {
                                if (
                                    !(((gpu.gpu_stat & 0x40) && (gpu.gpu_stat & 0x4)) |
                                      ((gpu.gpu_stat & 0x20) && (gpu.mode == 2)) |
                                      ((gpu.gpu_stat & 0x10) && (gpu.mode == 1)) |
                                      ((gpu.gpu_stat & 0x8) && (gpu.mode == 0))) // GPU STAT line is low
                                    &&
                                    (gpu.gpu_stat & 0x20))
                                {
                                        z80_p->int_f |= 0x2;
                                };
                                gpu.mode = 2;
                        }
                }
                break;

        // VBlank
        case 1:
                gpu.window_YC = false;
                if (gpu.mode_clock >= 456)
                {
                        gpu.mode_clock = 0;
                        gpu.line++;
                        if (
                            !(((gpu.gpu_stat & 0x40) && (gpu.gpu_stat & 0x4)) |
                              ((gpu.gpu_stat & 0x20) && (gpu.mode == 2)) |
                              ((gpu.gpu_stat & 0x10) && (gpu.mode == 1)) |
                              ((gpu.gpu_stat & 0x8) && (gpu.mode == 0))) // GPU STAT line is low
                            &&
                            ((gpu.line == gpu.lineYC) && (gpu.gpu_stat & 0x40)))
                        {
                                z80_p->int_f |= 0x2;
                        };
                        gpu.gpu_stat |= (((gpu.line == gpu.lineYC) ? 1 : 0) << 2);

                        if (gpu.line > 153)
                        {
                                // restart scanning mode
                                if (
                                    !(((gpu.gpu_stat & 0x40) && (gpu.gpu_stat & 0x4)) |
                                      ((gpu.gpu_stat & 0x20) && (gpu.mode == 2)) |
                                      ((gpu.gpu_stat & 0x10) && (gpu.mode == 1)) |
                                      ((gpu.gpu_stat & 0x8) && (gpu.mode == 0))) // GPU STAT line is low
                                    &&
                                    (gpu.gpu_stat & 0x20))
                                {
                                        z80_p->int_f |= 0x2;
                                };
                                gpu.mode = 2;
                                gpu.line = 0;
                                gpu.wdow_row = 0;
                                if (gpu.line == gpu.wdow_y)
                                        gpu.window_YC = true;
                        }
                }
                break;
        }
        // if (((gpu.mode == 0) && (gpu.gpu_stat & 0x8)) ||
        //     ((gpu.mode == 2) && (gpu.gpu_stat & 0x20)) ||
        //     ((gpu.mode == 1) && (gpu.gpu_stat & 0x10)))
        // {
        //         z80_p->int_f |= 0x2;
        // }
        gpu.gpu_stat = gpu.mode | (((gpu.line == gpu.lineYC) ? 1 : 0) << 2) |
                       (gpu.gpu_stat & 0xF8);
}

void renderscan()
{
        /* Obtain pixels */
        unsigned short mapoffs = GPU_BG_MAP ? 0x1C00 : 0x1800;
        unsigned char lineoffs = gpu.scrollX >> 3;
        unsigned char y = (gpu.line + gpu.scrollY) & 7;
        unsigned char x = gpu.scrollX & 7;
        unsigned char color, sprite_num, sprite_flags, tile_x, tile_y, sprite_size, i, j;
        unsigned short tile;
        unsigned char scanline_row[160];
        unsigned char sprite_row[160];

        if (gpu.line > 144)
        {
                return;
        }

        memset(sprite_row, 0, 160);
        for (i = 0; i < 160; i++)
        {
                pixels[i + (gpu.line) * 160] = PAL[0];
        }

        // mapoffs += (((gpu.line + gpu.scrollY) & 0xFF) >> 3) << 5;
        mapoffs += ((gpu.line + gpu.scrollY) & 0xF8) << 2;
        tile = (unsigned short)gpu.vram[mapoffs + lineoffs];

        if (!GPU_BG_SET && tile < 128)
                tile += 256;
        if (GPU_BG)
        {
                for (i = 0; i < 160; i++)
                {
                        color = gpu.bg_pal >> (gpu.tileset[tile][y][x] * 2);
                        color &= 3;

                        scanline_row[i] = gpu.tileset[tile][y][x];

                        pixels[i + (gpu.line) * 160] = PAL[color];

                        x++;
                        if (x == 8)
                        {
                                x = 0;
                                lineoffs = (lineoffs + 1) & 31;
                                tile = gpu.vram[mapoffs + lineoffs];
                                if (!GPU_BG_SET && tile < 128)
                                        tile += 256;
                        }
                }
                if (GPU_WDOW)
                {
                        mapoffs = GPU_WDOW_MAP ? 0x1C00 : 0x1800;
                        lineoffs = 0;
                        y = (gpu.line + gpu.wdow_y) & 7;

                        x = 0;

                        // mapoffs += (((gpu.wdow_row) & 0xFF) >> 3) << 5;
                        mapoffs += (gpu.wdow_row & 0xF8) << 2;

                        tile = (unsigned short)gpu.vram[mapoffs + lineoffs];

                        if (!GPU_BG_SET && tile < 128)
                                tile += 256;
                        if (gpu.wdow_y <= gpu.line)
                        {
                                for (i = gpu.wdow_x - 7; i < 160; i++)
                                {
                                        color = gpu.bg_pal >> (gpu.tileset[tile][y][x] * 2);
                                        color &= 3;

                                        scanline_row[i] = gpu.tileset[tile][y][x];

                                        pixels[i + (gpu.line) * 160] = PAL[color];

                                        x++;
                                        if (x == 8)
                                        {
                                                x = 0;
                                                lineoffs = (lineoffs + 1) & 31;

                                                tile = gpu.vram[mapoffs + lineoffs];
                                                if (!GPU_BG_SET && tile < 128)
                                                        tile += 256;
                                        }
                                }
                                if (gpu.wdow_x - 7 < 160)
                                        gpu.wdow_row++;
                        }
                }
        }

        if (GPU_SPR)
        {
                sprite_size = 1 + GPU_SP_SZ;
                for (j = 0; j < gpu.num_sprites; j++)
                {
                        i = gpu.sprites[j];
                        y = gpu.oam[4 * i] - 16;
                        x = gpu.oam[4 * i + 1] - 8;
                        sprite_num = gpu.oam[4 * i + 2];
                        if (GPU_SP_SZ)
                                sprite_num &= 0xFE;
                        sprite_flags = gpu.oam[4 * i + 3];

                        unsigned char pal;

                        if (GET_BIT(sprite_flags, 4))
                        {
                                pal = gpu.ob_pal1;
                        }
                        else
                        {
                                pal = gpu.ob_pal0;
                        }

                        if (y <= gpu.line && (y + sprite_size * 8) > gpu.line)
                        {
                                if (GET_BIT(sprite_flags, 6))
                                {
                                        tile_y = 7 - (gpu.line - y) + 8 * GPU_SP_SZ;
                                }
                                else
                                {
                                        tile_y = (gpu.line - y);
                                }

                                for (tile_x = 0; tile_x < 8; tile_x++)
                                {
                                        if ((x + tile_x) >= 0 &&
                                            (x + tile_x) < 160 &&
                                            (!GET_BIT(sprite_flags, 7) ||
                                             !scanline_row[x + tile_x]) &&
                                            !sprite_row[x + tile_x])
                                        {
                                                if (GET_BIT(sprite_flags, 5))
                                                {
                                                        color = gpu.tileset[sprite_num][tile_y][7 - tile_x];
                                                }
                                                else
                                                {
                                                        color = gpu.tileset[sprite_num][tile_y][tile_x];
                                                }

                                                if (color)
                                                {
                                                        color = pal >> (color * 2);
                                                        color &= 3;

                                                        pixels[x + tile_x + (gpu.line) * 160] = PAL[color];
                                                }
                                                sprite_row[x + tile_x] = 1;
                                        }
                                }
                        }
                }
        }
}

void showBGMap()
{
        unsigned char x, y, color;
        unsigned short tile, tile_addr;
        unsigned short mapoffs = GPU_BG_MAP ? 0x1C00 : 0x1800;

        for (tile = 0; tile < 1024; tile++)
        {
                tile_addr = gpu.vram[mapoffs + tile];
                if (!GPU_BG_SET && tile_addr < 128)
                        tile_addr += 256;

                for (x = 0; x < 8; x++)
                {
                        for (y = 0; y < 8; y++)
                        {
                                color = gpu.bg_pal >> (gpu.tileset[tile_addr][y][x] * 2);
                                color &= 3;

                                pixels_bg[((tile / 32) * 8 + y) * 256 + (tile % 32) * 8 + x] = PAL[color];
                        }
                }
        }
        SDL_UpdateTexture(framebuffer_bgmap, NULL, pixels_bg, 256 * sizeof(unsigned int));
        SDL_RenderCopy(bg_r, framebuffer_bgmap, NULL, NULL);

        SDL_Rect screen1 = {gpu.scrollX, gpu.scrollY, 160, 144};
        SDL_Rect screen2 = {gpu.scrollX, gpu.scrollY, 160, 144};
        SDL_Rect screen3 = {gpu.scrollX, gpu.scrollY, 160, 144};
        SDL_Rect screen4 = {gpu.scrollX, gpu.scrollY, 160, 144};

        if (screen1.x + screen1.w > 256)
                screen2.x -= 256;
        if (screen1.y + screen1.h > 256)
                screen3.y -= 256;
        if (screen1.x + screen1.w > 256 && screen1.y + screen1.h > 256)
                screen4.x -= 256;
        screen4.y -= 256;
        SDL_SetRenderDrawColor(bg_r, 255, 0, 0, 255);
        SDL_RenderDrawRect(bg_r, &screen1);
        SDL_RenderDrawRect(bg_r, &screen2);
        SDL_RenderDrawRect(bg_r, &screen3);
        SDL_RenderDrawRect(bg_r, &screen4);
        SDL_RenderPresent(bg_r);
}

void showTileSet()
{
        unsigned char x, y, color;
        unsigned short tile;

        memset(pixels_vram, 0xFF, (9 * 16 - 1) * (9 * 24 - 1) * sizeof(unsigned int));

        for (tile = 0; tile < 384; tile++)
        {
                for (x = 0; x < 8; x++)
                {
                        for (y = 0; y < 8; y++)
                        {
                                color = gpu.bg_pal >> (gpu.tileset[tile][y][x] * 2);
                                color &= 3;

                                // tile / 16 = tile row
                                // tile row * 9 + y
                                pixels_vram[((tile / 16) * 9 + y) * (9 * 16 - 1) + (tile % 16) * 9 + x] = PAL[color];
                        }
                }
        }
        SDL_UpdateTexture(framebuffer_tileset, NULL, pixels_vram, (9 * 16 - 1) * sizeof(unsigned int));
        SDL_RenderCopy(vram_r, framebuffer_tileset, NULL, NULL);
        SDL_RenderPresent(vram_r);
}

void setup(bool tileset, bool bgmap)
{
        gpu.gpu_ctrl = 0;
        int i, x, y;
        SDL_Point window_size = {160, 144};

        show_tileset = tileset;
        show_bgmap = bgmap;

        window = SDL_CreateWindow("gbc_cpp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_size.x * 2, window_size.y * 2, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        SDL_RenderSetLogicalSize(renderer, 160, 144);
        SDL_GetWindowPosition(window, &x, &y);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

        if (tileset)
        {
                SDL_CreateWindowAndRenderer(
                    18 * 16 - 2, 18 * 24 - 2, 0, &vram_w, &vram_r);
                SDL_RenderSetLogicalSize(vram_r, 9 * 16 - 1, 9 * 24 - 1);
                SDL_SetWindowPosition(vram_w, x - 18 * 16 + 2, y);
                SDL_SetRenderDrawColor(vram_r, 255, 253, 208, 255);
                SDL_RenderClear(vram_r);
                SDL_RenderPresent(vram_r);
                framebuffer_tileset = SDL_CreateTexture(vram_r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 9 * 16 - 1, 9 * 24 - 1);
        }

        if (bgmap)
        {
                SDL_CreateWindowAndRenderer(
                    256 * 2, 256 * 2, 0,
                    &bg_w, &bg_r);
                SDL_RenderSetLogicalSize(bg_r, 256, 256);
                SDL_SetWindowPosition(bg_w, x + 160 * 2, y);
                SDL_SetRenderDrawColor(bg_r, 255, 253, 208, 255);
                SDL_RenderClear(bg_r);
                SDL_RenderPresent(bg_r);
                framebuffer_bgmap = SDL_CreateTexture(bg_r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 256);
        }

        for (i = 0; i < 100; i++)
        {
                SDL_Delay(1);
                SDL_PumpEvents();
        }
        SDL_RaiseWindow(window);
}

void cleanup()
{
        SDL_DestroyWindow(window);
        SDL_DestroyWindow(vram_w);
        SDL_DestroyWindow(bg_w);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyRenderer(vram_r);
        SDL_DestroyRenderer(bg_r);
        SDL_DestroyTexture(framebuffer);
        SDL_DestroyTexture(framebuffer_bgmap);
        SDL_DestroyTexture(framebuffer_tileset);
        SDL_Quit();
}
