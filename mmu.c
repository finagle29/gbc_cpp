#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "gbz80.h"
#include "mmu.h"
#include "gpu.h"
#include "keys.h"
#include "apu.h"
#include "gb_apu/GBAPU_Wrapper.h"


mmu_type *mmu = NULL;

char save_fname[50];


void init_mmu() {

        unsigned short addr = 0;
        for (addr = 0; addr < 0x8000; addr++) {
                wb(addr, 0);
        }
}

void print_n_bytes(unsigned short n) {
        unsigned short i = 0;
        while (i < n) {
                printf("%x\n", rb(i++));
        }
}

void save_sram() {
        printf("saving to %s\n", save_fname);
        FILE *f = fopen(save_fname, "w+b");
        fwrite(mmu->eram, sizeof(unsigned char), sizeof(mmu->eram), f);
        fclose(f);
}

void load_bios(char* fname) {
        FILE *f = fopen(fname, "rb");
        fread(mmu->bios, sizeof(unsigned char), 0x100, f);
        fclose(f);
}

void load_rom(char* fname) {
        char fname_copy[50];
        strcpy(fname_copy, fname);
        char *base_name = strtok(fname_copy, ".");
        if (base_name != NULL) {
                snprintf(save_fname, 50, "%s.sav", base_name);
        }
        unsigned char cart_type;

        FILE *f = fopen(fname, "rb");
        unsigned long rom_size;

        fseek(f, 0, SEEK_END);
        rom_size = (unsigned long)ftell(f);
        mmu = (mmu_type *)malloc(sizeof(mmu_type) + rom_size);
        
        fseek(f, 0x147, SEEK_SET);
        switch (cart_type = fgetc(f))
        {
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x08: case 0x09:
                        mmu->mbc = 1;
                        break;
                case 0x05: case 0x06:
                        mmu->mbc = 2;
                        break;
                case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
                        mmu->mbc = 3;
                        break;
                default:
                        mmu->mbc = 1;
        }
        printf("using mbc%d\n", mmu->mbc);

        rewind(f);

        mmu->mode = 0;
        
        if (mmu == NULL) {
                fprintf(stderr, "Error using malloc for mmu.");
                fclose(f);
                exit(1);
        }


        fseek(f, 0, SEEK_SET);

        fread(mmu->rom, sizeof(unsigned char), rom_size, f);
        fclose(f);

        struct stat buffer;
        if (stat(save_fname, &buffer) == 0) {
                printf("reading save file %s ...\n", save_fname);
                f = fopen(save_fname, "rb");
                size_t result = fread(mmu->eram, sizeof(unsigned char), sizeof(mmu->eram), f);
                printf("read %lu bytes\n", result);
                fclose(f);
        } else {
                printf("save file %s does not exist\n", save_fname);
                memset(mmu->eram, 0, 0x20000);
        }

        mmu->rom_bank = 1;
        mmu->ram_bank = 0;
        mmu->eram_enable = false;
        mmu->inbios = true;

        memset(mmu->vram, 0, 0x2000);
        memset(mmu->wram, 0, 0x2000);
        memset(mmu->zram, 0, 0x80);
        memset(mmu->io, 0, 0x80);
}

unsigned char rb(unsigned short addr) {
        switch (addr & 0xF000) {
                /* BIOS (256b) or ROM0 */
                case 0x0000:
                        if (mmu->inbios)
                                if (addr < 0x0100)
                                        return mmu->bios[addr];
                        return mmu->rom[addr];
                case 0x1000:
                case 0x2000:
                case 0x3000:
                        return mmu->rom[addr];

                /* ROM1 (banked) (16k) */
                case 0x4000:
                case 0x5000:
                case 0x6000:
                case 0x7000:
                        return mmu->rom[mmu->rom_bank * 0x4000 + (addr & 0x3FFF)];

                /* Graphics: VRAM (8k) */
                case 0x8000:
                case 0x9000:
                        if (gpu.mode == 3) return 0xFF;
                        return gpu.vram[addr & 0x1FFF];

                /* External RAM (8k) */
                case 0xA000:
                case 0xB000:
                        if (mmu->eram_enable)
                                return mmu->eram[mmu->ram_bank * 0x2000 + (addr & 0x1FFF)];
                        return 0xFF;

                /* Working RAM (8k) */
                case 0xC000:
                case 0xD000:
                        return mmu->wram[(addr & 0x1FFF)];

                /* Working RAM shadow (8k) */
                case 0xE000:
                        return mmu->wram[addr & 0x1FFF];
                /* Working RAM shadow, I/O, zero-page RAM */
                case 0xF000:
                        switch (addr & 0x0F00)
                        {
                                /* Working RAM shadow */
                                case 0x000: case 0x100: case 0x200: case 0x300:
                                case 0x400: case 0x500: case 0x600: case 0x700:
                                case 0x800: case 0x900: case 0xA00: case 0xB00:
                                case 0xC00: case 0xD00:
                                        return mmu->wram[addr & 0x1FFF];

                                case 0xE00:
                                        if (addr < 0xFEA0) {
                                                if (gpu.mode & 0x2) return 0xFF;
                                                return gpu.oam[addr & 0xFF];
                                        } else
                                                return 0;
                                case 0xF00:
                                        if (addr >= 0xFF80) {
                                                return mmu->zram[addr & 0x7F];
                                        } else if (addr >= Gb_Apu_start_addr &&
                                                       addr <= Gb_Apu_end_addr) {
                                                return Gb_Apu_read_register(apu,
                                                                z80.clock.m,
                                                                addr);
                                        } else {
                                        switch (addr) {
                                        case 0xFF00:
                                                return key.rows[key.column];
                                        case 0xFF04:
                                                return z80_p->clock.div;
                                        case 0xFF05:
                                                return z80_p->clock.tima;
                                        case 0xFF06:
                                                return z80_p->clock.tma;
                                        case 0xFF07:
                                                return z80_p->clock.tac;
                                        case 0xFF0F:
                                                return z80_p->int_f;
                                        case 0xFF40:
                                                return gpu.gpu_ctrl;
                                        case 0xFF41:
                                                return gpu.gpu_stat;
                                        case 0xFF42:
                                                return gpu.scrollY;
                                        case 0xFF43:
                                                return gpu.scrollX;
                                        case 0xFF44:
                                                return gpu.line;
                                        case 0xFF45:
                                                return gpu.lineYC;
                                        case 0xFF46:
                                                return gpu.DMA;
                                        case 0xFF47:
                                                return gpu.bg_pal;
                                        case 0xFF48:
                                                return gpu.ob_pal0;
                                        case 0xFF49:
                                                return gpu.ob_pal1;
                                        case 0xFF4A:
                                                return gpu.wdow_y;
                                        case 0xFF4B:
                                                return gpu.wdow_x;
                                        case 0xFFFF:
                                                return z80_p->int_en;
                                        default:
                                                return mmu->io[addr & 0x7F];
                                                /* TODO: handle I/O */
                                        }
                                        }
                        }
        }
        return 0;
}

unsigned short rw(unsigned short addr) {
        return (unsigned short)(rb(addr) | (rb(addr + 1) << 8));
}

void ww(unsigned short addr, unsigned short val) {
        wb(addr, val & 0xFF);
        wb(addr + 1, val >> 8);
}

// TODO: MBC when writing to ROM
void wb(unsigned short addr, unsigned char val) {
        switch (addr & 0xF000) {
                /* BIOS (256b) or ROM0 */
                case 0x0000:
                case 0x1000:
                        if (mmu->mbc & 1) {
                                mmu->eram_enable = ((val & 0x0F) == 0x0A);
                                if (!mmu->eram_enable) {
                                        save_sram();
                                }
                        } else if ((mmu->mbc == 2) && ((addr & 0x100) == 0)) {
                                mmu->eram_enable = ((val & 0x0F) == 0x0A);
                                if (!mmu->eram_enable) {
                                        save_sram();
                                }
                        }
                        break;
                case 0x2000:
                case 0x3000:
                        if (mmu->mbc == 1) {
                                mmu->rom_bank = (mmu->rom_bank & ~0x1F) | ((val ? val : val + 1) & 0x1F);
                        } else if (mmu->mbc == 3) {
                                mmu->rom_bank = (val ? val : val + 1)  & 0x7F;
                        } else if (mmu->mbc == 2 && (addr & 0x100)) {
                                mmu->rom_bank = val & 0x0F;
                        }
                        break;

                /* ROM1 (unbanked) (16k) */
                case 0x4000:
                case 0x5000:
                        if (mmu->mbc == 1) {
                                if (mmu->mode) {
                                        mmu->ram_bank = val & 0x3;
                                } else {
                                        mmu->rom_bank = (unsigned char)((mmu->rom_bank & 0x1F) | ((val & 0x3) << 5));
                                }
                        } else if (mmu->mbc == 3) {
                                mmu->ram_bank = val & 0x7;
                        }
                        break;
                case 0x6000:
                case 0x7000:
                        if (mmu->mbc == 1) {
                                mmu->mode = val & 1; // MBC 1
                            if (mmu->mode) {
                                    mmu->rom_bank &= 0x1F;
                            } else {
                                    mmu->ram_bank = 0;
                            }
                        }
                        break;

                /* Graphics: VRAM (8k) */
                case 0x8000:
                case 0x9000:
                        if (gpu.mode == 3) return;
                        gpu.vram[addr & 0x1FFF] = val;
                        if (addr <= 0x97FF) {
                                unsigned short tmp_addr = addr & 0x1FFE;
                                unsigned short tile = (tmp_addr >> 4) & 511;
                                unsigned char y = (tmp_addr >> 1) & 7;
                                unsigned char sx, x;
                                for (x = 0; x < 8; x++) {
                                        sx = (unsigned char)(1 << (7 - x));
                                        gpu.tileset[tile][y][x] = 
                                                ((gpu.vram[tmp_addr] & sx) ? 1 : 0) +
                                                ((gpu.vram[tmp_addr+1] & sx) ? 2 : 0);
                                }
                        }
                        break;

                /* External RAM (8k) */
                case 0xA000:
                case 0xB000:
                        mmu->eram[mmu->ram_bank * 0x2000 + (addr & 0x1FFF)] = val;
                        break;

                /* Working RAM (8k) */
                case 0xC000:
                case 0xD000:
                        mmu->wram[addr & 0x1FFF] = val;
                        break;

                /* Working RAM shadow (8k) */
                case 0xE000:
                        mmu->wram[addr & 0x1FFF] = val;
                        break;
                /* Working RAM shadow, I/O, zero-page RAM */
                case 0xF000:
                        switch (addr & 0x0F00)
                        {
                                /* Working RAM shadow */
                                case 0x000: case 0x100: case 0x200: case 0x300:
                                case 0x400: case 0x500: case 0x600: case 0x700:
                                case 0x800: case 0x900: case 0xA00: case 0xB00:
                                case 0xC00: case 0xD00:
                                        mmu->wram[addr & 0x1FFF] = val;
                                        break;

                                case 0xE00:
                                        if (addr < 0xFEA0) {
                                                if (gpu.mode & 0x2) return;
                                                gpu.oam[addr & 0xFF] = val;
                                        } 
                                        break;
                                case 0xF00:
                                        if (addr >= 0xFF80) {
                                                mmu->zram[addr & 0x7F] = val;
                                        } else {
                                                mmu->io[addr & 0x7F] = val;
                                        }
                                        if (addr >= Gb_Apu_start_addr &&
                                                        addr <= Gb_Apu_end_addr) {
                                                Gb_Apu_write_register(apu,
                                                                z80.clock.m,
                                                                addr, val);
                                        }
                                        switch (addr) {
                                        case 0xFF00:
                                                key.column = ((val & 0x30) >> 4);
                                                break;
                                        case 0xFF02:
                                                if (val == 0x81) {
                                                        printf("%c", mmu->io[1]);
                                                        fflush(stdout);
                                                }
                                                break;
                                        case 0xFF04:
                                                z80_p->clock.div = 0;
                                                break;
                                        case 0xFF05:
                                                z80_p->clock.tima = val;
                                                break;
                                        case 0xFF06:
                                                z80_p->clock.tma = val;
                                                break;
                                        case 0xFF07:
                                                z80_p->clock.old_tac = z80_p->clock.tac;
                                                z80_p->clock.tac = val;
                                                break;
                                        case 0xFF0F:
                                                z80_p->int_f = val;
                                                break;
                                        case 0xFF40:
                                                gpu.gpu_ctrl = val;
                                                break;
                                        case 0xFF41:
                                                gpu.gpu_stat = val;
                                                break;
                                        case 0xFF42:
                                                gpu.scrollY = val;
                                                break;
                                        case 0xFF43:
                                                gpu.scrollX = val;
                                                break;
                                        case 0xFF44:
                                                gpu.line = val;
                                                break;
                                        case 0xFF45:
                                                gpu.lineYC = val;
                                                break;
                                        case 0xFF46:
                                                gpu.DMA = val;
                                                gpu.do_DMA = true;
                                                break;
                                        case 0xFF47:
                                                gpu.bg_pal = val;
                                                break;
                                        case 0xFF48:
                                                gpu.ob_pal0 = val;
                                                break;
                                        case 0xFF49:
                                                gpu.ob_pal1 = val;
                                                break;
                                        case 0xFF4A:
                                                gpu.wdow_y = val;
                                                break;
                                        case 0xFF4B:
                                                gpu.wdow_x = val;
                                                break;
                                        case 0xFF50:
                                                mmu->inbios = false;
                                                break;
                                        case 0xFFFF:
                                                z80_p->int_en = val;
                                                break;
                                        default:
                                                break;
                                        }
                        }
        }
}

