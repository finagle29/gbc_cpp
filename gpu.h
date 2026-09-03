#ifndef GPU_H
#define GPU_H

#include <stdbool.h>
#include <SDL.h>

#define GPU_BG GET_BIT(gpu.gpu_ctrl, 0)
#define GPU_SPR GET_BIT(gpu.gpu_ctrl, 1)
#define GPU_SP_SZ GET_BIT(gpu.gpu_ctrl, 2)
#define GPU_BG_MAP GET_BIT(gpu.gpu_ctrl, 3)
#define GPU_BG_SET GET_BIT(gpu.gpu_ctrl, 4)
#define GPU_WDOW GET_BIT(gpu.gpu_ctrl, 5)
#define GPU_WDOW_MAP GET_BIT(gpu.gpu_ctrl, 6)
#define GPU_DISP GET_BIT(gpu.gpu_ctrl, 7)

void init_gpu(void);
void renderscan(void);
void dump_vram(void);
void showTileSet(void);
void showBGMap(void);

typedef struct
{
        unsigned char color, palette, sprite_priority;
        bool bg_priority;
} FIFO_item_t;

typedef struct
{
        unsigned char start, end, size;
        // unsigned char FIFO[16];
        FIFO_item_t FIFO[16];
} FIFO_t;

typedef struct
{
        unsigned char x, mode, tile, tile_hi, tile_lo;
        bool starting, window, sprite;
} fetcher_t;

typedef struct
{
        unsigned char oam[0xA0];
        unsigned char vram[0x2000];

        unsigned char gpu_ctrl, gpu_stat;
        unsigned char scrollX, scrollY;
        unsigned char mode;
        unsigned char line, lineYC, x, trashed_pixels;
        unsigned char DMA, DMA_ptr;
        unsigned short mode_clock;
        bool do_DMA, DMA_requested, reset_DMA, window_YC;
        unsigned char bg_pal, ob_pal0, ob_pal1;
        unsigned char wdow_y, wdow_x, wdow_row;
        unsigned char num_sprites, sprite_ix;
        unsigned char sprites[10];
        // unsigned short timer;

        FIFO_t bg_FIFO, sprite_FIFO;
        fetcher_t fetcher;

        unsigned char tileset[384][8][8];
} gpu_type;

extern gpu_type gpu;
extern SDL_Window *window, *vram_w, *bg_w;
extern bool show_tileset, show_bgmap;

void setup(bool, bool);
void cleanup(void);

void gpu_m_tick(void);

FIFO_item_t FIFO_pop(FIFO_t *fifo);
void FIFO_push(FIFO_t *fifo, FIFO_item_t item);

#endif
