#ifndef GPU_H
#define GPU_H

#include <stdbool.h>

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

void fetch_pixels_FIFO(void);

typedef struct {
        // internal state goes here
        unsigned char start;
        unsigned char end;
        unsigned char size;
        unsigned char FIFO[16];
} FIFO_t;

typedef struct {
        unsigned char mode;
        unsigned short tile;
        unsigned char xoffs;
        unsigned char pixels[8];
        bool ready;
} fetcher_t;

typedef struct {
        unsigned char oam[0xA0];
        unsigned char vram[0x2000];

        unsigned char gpu_ctrl, gpu_stat;
        unsigned char scrollX, scrollY;
        unsigned char mode;
        unsigned char line, lineYC;
        unsigned char DMA, DMA_ptr;
        unsigned short mode_clock;
        bool do_DMA;
        unsigned char bg_pal, ob_pal0, ob_pal1;
        unsigned char wdow_y, wdow_x;

        unsigned char tileset[384][8][8];

        unsigned char xpos;
        FIFO_t sprite_FIFO;
        FIFO_t bg_FIFO;
        fetcher_t fetcher;
} gpu_type;

extern gpu_type gpu;

void setup(bool, bool);
void cleanup(void);

void gpu_step(void);
void push_pixel(void);
unsigned char FIFO_pop(FIFO_t* fifo);



#endif
