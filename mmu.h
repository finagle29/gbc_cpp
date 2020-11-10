/* Header file for MMU of GameBoy */

#ifndef MMU_H_
#define MMU_H_

#include <stdbool.h>

void init_mmu(void);
void load_bios(char *f);
void print_n_bytes(unsigned short n);
void save_sram(void);
void load_rom(char *f);

unsigned char rb(unsigned short addr);
unsigned short rw(unsigned short addr);

void wb(unsigned short addr, unsigned char val);
void ww(unsigned short addr, unsigned short val);

typedef struct {
        unsigned char last_latch_write;
        unsigned char seconds;
        unsigned char minutes;
        unsigned char hours;
        unsigned short day_counter;
        bool halt_flag, day_counter_carry;
} rtc_type;

typedef struct {
        bool inbios, eram_enable;

        unsigned short rom_bank, rom_banks;
        unsigned char ram_bank, ram_banks;
        unsigned char mode, mbc; // mode = 0 means MBC1 is 16/8
                                 // mode = 1 means MBC1 is 4/32
        rtc_type rtc;

        unsigned char bios[0x100];
        unsigned char vram[0x2000];
        unsigned char eram[0x20000];
        unsigned char wram[0x2000];
        unsigned char zram[0x80];
        unsigned char io[0x80];
        unsigned char rom[];
} mmu_type;

extern mmu_type *mmu;

extern char save_fname[50];

#endif
